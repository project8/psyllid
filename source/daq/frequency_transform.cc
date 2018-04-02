
/*
 * frequency_transform.cc
 *
 *  Created on: March 28, 2018
 *      Author: laroque
 */

#include "frequency_transform.hh"

//#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <thread>
#include <memory>
#include <sys/types.h> // for ssize_t

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( frequency_transform, "frequency-transform", frequency_transform_binding );

    LOGGER( plog, "frequency_transform" );

    frequency_transform::frequency_transform() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            f_fft_size( 4096 ), //TODO is this a reasonable default
            f_start_paused( true ),
            f_transform_flag( "ESTIMATE" ), //TODO is this a reasonable default?
            f_use_wisdom( true ),
            f_wisdom_filename( "wisdom_complexfft.fftw3" ),
            f_transform_flag_map(),
            f_fftw_input(),
            f_fftw_output(),
            f_fftw_plan(),
            f_paused( true ),
            f_multithreaded_is_initialized( false )
            //TODO we maybe want these?
            //f_time_session_pkt_counter( 0 ),
            //f_freq_session_pkt_counter( 0 )
    {
        setup_internal_maps();
    }

    frequency_transform::~frequency_transform()
    {
    }

    void frequency_transform::initialize()
    {
        out_buffer< 0 >().initialize( f_time_length );
        out_buffer< 1 >().initialize( f_freq_length );

        // fftw stuff
        TransformFlagMap::const_iterator iter = f_transform_flag_map.find(f_transform_flag);
        unsigned transform_flag = iter->second;
        // initialize FFTW IO arrays
        f_fftw_input = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * f_fft_size);
        f_fftw_output = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * f_fft_size);
        if (f_use_wisdom)
        {
            LDEBUG( plog, "Reading wisdom from file <" << f_wisdom_filename << ">");
            if (fftw_import_wisdom_from_filename(f_wisdom_filename.c_str()) == 0)
            {
                LWARN( plog, "Unable to read FFTW wisdom from file <" << f_wisdom_filename << ">" );
            }
        }
        //initialize multithreaded
        #ifdef FFTW_NTHREADS
        if (! f_multithreaded_is_initialized)
        {
            fftw_init_threads();
            fftw_plan_with_nthreads(FFTW_NTHREADS);
            LDEBUG( plog, "Configuring FFTW to use up to " << FFTW_NTHREADS << " threads.");
            f_multithreaded_is_initialized = true;
        }
        #endif
        //create fftw plan
        f_fftw_plan = fftw_plan_dft_1d(f_fft_size, f_fftw_input, f_fftw_output, FFTW_FORWARD, transform_flag | FFTW_PRESERVE_INPUT);
        //save plan
        if (f_fftw_plan != NULL)
        {
            if (f_use_wisdom)
            {
                if (fftw_export_wisdom_to_filename(f_wisdom_filename.c_str()) == 0)
                {
                    LWARN( plog, "Unable to write FFTW wisdom to file<" << f_wisdom_filename << ">");
                }
            }
            LDEBUG( plog, "FFTW plan created; initialization complete" );
        }

        return;
    }

    void frequency_transform::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the frequency transformer" );

            time_data* time_data_in = nullptr;
            time_data* time_data_out = nullptr;
            freq_data* freq_data_out = nullptr;

            f_paused = f_start_paused;

            //uint64_t t_time_batch_pkt = 0;
            //uint64_t t_freq_batch_pkt = 0;

            // starting in a non-paused state is not known to work
            if( ! f_start_paused )
            {
                LDEBUG( plog, "FREQUENCY TRANSFORM starting unpaused" );
                out_stream< 0 >().data()->set_pkt_in_session( 0 );
                out_stream< 1 >().data()->set_pkt_in_session( 0 );
                if( ! out_stream< 0 >().set( stream::s_start ) ) return;
                if( ! out_stream< 1 >().set( stream::s_start ) ) return;
                //TODO need this?
                //f_time_session_pkt_counter = 0;
                //f_freq_session_pkt_counter = 0;
            }

            try
            {
                LPROG( plog, "Starting main loop (frequency transform)" );
                while (! is_canceled() )
                {
                    if (out_stream< 0 >().get() == stream::s_stop || out_stream< 1 >().get() == stream::s_stop)
                    {
                        LWARN( plog, "output stream(s) have stop condition" );
                        break;
                    }
                    // check for [un]pause instruction
                    if( have_instruction() )
                    {
                        if( f_paused && use_instruction() == midge::instruction::resume )
                        {
                            LDEBUG( plog, "freq transform resuming" );
                            if( ! out_stream< 0 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 0 error while starting";
                            if( ! out_stream< 1 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 1 error while starting";
                            f_paused = false;
                        }
                        else if( !f_paused && use_instruction() == midge::instruction::pause )
                        {
                            LDEBUG( plog, "freq transform pausing" );
                            if( ! out_stream< 0 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 0 error while stopping";
                            if( ! out_stream< 1 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 1 error while stopping";
                            f_paused = true;
                        }
                    }

                    if ( !f_paused )
                    {
                        time_data_in = in_stream< 0 >().data();
                        time_data_out = out_stream< 0 >().data();
                        freq_data_out = out_stream< 1 >().data();

                        //TODO can i really just do this dereference thing?
                        //     seems like i should need to do something harder, like the commented memcpy or something
                        *time_data_out = *time_data_in;
                        //std::memcpy(time_data_out->get_array(), time_data_in->get_array(), time_data_in->get_array_size());

                        //// okay so this is where the real magic happens, should do the FFT bit here
                        // memcpy just to place some bits so i can do the rest of the logic independent of the FFTW API
                        std::memcpy(freq_data_out->get_array(), time_data_in->get_array(), time_data_in->get_array_size());

                        if (!out_stream< 0 >().set( stream::s_run ) || !out_stream< 1 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "frequency transform error setting output stream to s_run" );
                            break;
                        }

                        //TODO this is based on tf_roach_receiver, why do this at the end of the loop and not the top?
                        //     i clearly still don't have a great grasp on how these streams work
                        in_stream< 0 >().get();
                    }
                }
            }
            catch( error& e )
            {
                throw;
            }

            LINFO( plog, "FREQUENCY TRANSFORM is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output streams" );
            bool t_stop_ok = out_stream< 0 >().set( stream::s_stop ) && out_stream< 1 >().set( stream::s_stop );
            if( ! t_stop_ok ) return;

            LDEBUG( plog, "Exiting output streams" );
            if( ! out_stream< 0 >().set( stream::s_exit ) ) return;
            out_stream< 1 >().set( stream::s_exit );

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }

        return;
    }

    void frequency_transform::finalize()
    {
        out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        if (f_fftw_input != NULL )
        {
            fftw_free(f_fftw_input);
            f_fftw_input = NULL;
        }
        if (f_fftw_output != NULL)
        {
            fftw_free(f_fftw_output);
            f_fftw_output = NULL;
        }
        return;
    }

    void frequency_transform::setup_internal_maps()
    {
        f_transform_flag_map.clear();
        f_transform_flag_map["ESTIMATE"] = FFTW_ESTIMATE;
        f_transform_flag_map["MEASURE"] = FFTW_MEASURE;
        f_transform_flag_map["PATIENT"] = FFTW_PATIENT;
        f_transform_flag_map["EXHAUSTIVE"] = FFTW_EXHAUSTIVE;
    }


    // frequency_transform_binding methods
    frequency_transform_binding::frequency_transform_binding() :
            _node_binding< frequency_transform, frequency_transform_binding >()
    {
    }

    frequency_transform_binding::~frequency_transform_binding()
    {
    }

    void frequency_transform_binding::do_apply_config( frequency_transform* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring frequency_transform with:\n" << a_config );
        a_node->set_time_length( a_config.get_value( "time-length", a_node->get_time_length() ) );
        a_node->set_freq_length( a_config.get_value( "freq-length", a_node->get_freq_length() ) );
        a_node->set_start_paused( a_config.get_value( "start-paused", a_node->get_start_paused() ) );
        return;
    }

    void frequency_transform_binding::do_dump_config( const frequency_transform* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping frequency_transform configuration" );
        a_config.add( "time-length", new scarab::param_value( a_node->get_time_length() ) );
        a_config.add( "freq-length", new scarab::param_value( a_node->get_freq_length() ) );
        a_config.add( "start-paused", new scarab::param_value( a_node->get_start_paused() ) );
        return;
    }

    //bool frequency_transform_binding::do_run_command( frequency_transform* a_node, const std::string& a_cmd, const scarab::param_node& ) const
    //{
    //    if( a_cmd == "freq-only" )
    //    {
    //        a_node->switch_to_freq_only();
    //        return true;
    //    }
    //    else if( a_cmd == "time-and-freq" )
    //    {
    //        a_node->switch_to_time_and_freq();
    //        return true;
    //    }
    //    else
    //    {
    //        LWARN( plog, "Unrecognized command: <" << a_cmd << ">" );
    //        return false;
    //    }
    //}

} /* namespace psyllid */
