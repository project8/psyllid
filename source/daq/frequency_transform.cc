
/*
 * frequency_transform.cc
 *
 *  Created on: March 28, 2018
 *      Author: laroque
 */

#include "frequency_transform.hh"


#include "logger.hh"
#include "param.hh"

#include <cmath>
#include <thread>
#include <memory>
#include <sys/types.h>


using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( frequency_transform, "frequency-transform", frequency_transform_binding );

    LOGGER( plog, "frequency_transform" );

    frequency_transform::frequency_transform() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            f_fft_size( 4096 ),
            f_transform_flag( "ESTIMATE" ),
            f_use_wisdom( true ),
            f_wisdom_filename( "wisdom_complexfft.fftw3" ),
            f_enable_time_output( true ),
            f_transform_flag_map(),
            f_fftw_input(),
            f_fftw_output(),
            f_fftw_plan(),
            f_multithreaded_is_initialized( false )
    {
        setup_internal_maps();
    }

    frequency_transform::~frequency_transform()
    {
    }

    void frequency_transform::switch_to_freq_only()
    {
        LDEBUG( plog, "switching to frequency output only mode" );
        f_enable_time_output = false;
    }

    void frequency_transform::switch_to_time_and_freq()
    {
        LDEBUG( plog, "switching to frequency and time output mode" );
        f_enable_time_output = true;
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
            double fft_norm = sqrt(1. / (double)f_fft_size);

            try
            {
                LINFO( plog, "Starting main loop (frequency transform)" );
                while (! is_canceled() )
                {
                    // stop if output stream buffers have s_stop
                    if (f_enable_time_output && out_stream< 0 >().get() == stream::s_stop)
                    {
                        LWARN( plog, "time output stream has stop condition" );
                        break;
                    }
                    if (out_stream< 1 >().get() == stream::s_stop)
                    {
                        LWARN( plog, "frequency output stream has stop condition" );
                        break;
                    }
                    // grab the next input data and check slot status
                    midge::enum_t in_cmd = stream::s_none;
                    in_cmd = in_stream< 0 >().get();
                    if ( in_cmd == stream::s_none)
                    {
                        continue;
                    }
                    if ( in_cmd == stream::s_error )
                    {
                        LDEBUG( plog, "got an s_error on slot <" << in_stream< 0 >().get_current_index() << ">" );
                        break;
                    }
                    if ( in_cmd == stream::s_exit )
                    {
                        LDEBUG( plog, "got an s_exit on slot <" << in_stream< 0 >().get_current_index() << ">" );
                        break;
                    }
                    if ( in_cmd == stream::s_stop )
                    {
                        LDEBUG( plog, "got an s_stop on slot <" << in_stream< 0 >().get_current_index() << ">" );
                        if ( f_enable_time_output && ! out_stream< 0 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 0 error while stopping";
                        if ( ! out_stream< 1 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 1 error while stopping";
                        continue;
                    }
                    if ( in_cmd == stream::s_start )
                    {
                        LDEBUG( plog, "got an s_start on slot <" << in_stream< 0 >().get_current_index() << ">" );
                        if ( f_enable_time_output && ! out_stream< 0 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 0 error while starting";
                        if ( ! out_stream< 1 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 1 error while starting";
                        continue;
                    }
                    if ( in_cmd == stream::s_run )
                    {
                        LDEBUG( plog, "got an s_run on slot <" << in_stream< 0 >().get_current_index() << ">" );
                        time_data_in = in_stream< 0 >().data();

                        //time output
                        if (f_enable_time_output)
                        {
                            time_data_out = out_stream< 0 >().data();
                            *time_data_out = *time_data_in;
                        }
                        //frequency output
                        freq_data_out = out_stream< 1 >().data();
                        freq_data_out->set_freq_not_time( true );
                        //TODO there are many other members of the underlying roach_packet_data type; should more carefully think through all of them and if they should be on the frequency_data (is there a way to copy all the members *except* the data array?)
                        LDEBUG( plog, "next steams acquired, doing FFT" );
                        std::copy(&time_data_in->get_array()[0][0], &time_data_in->get_array()[0][0] + f_fft_size*2, &f_fftw_input[0][0]);
                        fftw_execute_dft(f_fftw_plan, f_fftw_input, f_fftw_output);
                        //is this the normalization we want? (is it what the ROACH does?)
                        for (size_t i_bin=0; i_bin<f_fft_size; ++i_bin)
                        {
                            f_fftw_output[i_bin][0] *= fft_norm;
                            f_fftw_output[i_bin][1] *= fft_norm;
                        }
                        //std::copy(&f_fftw_output[0][0], &f_fftw_output[0][0] + f_fft_size*2, &freq_data_out->get_array()[0][0]);
                        // FFT unfolding based on katydid:Source/Data/Transform/KTFrequencyTransformFFTW
                        unsigned t_center_bin = time_data_in->get_array_size();
                        std::copy(&f_fftw_output[0][0], &f_fftw_output[0][0] + (t_center_bin - 1), &freq_data_out->get_array()[0][0] + t_center_bin);
                        std::copy(&f_fftw_output[0][0] + t_center_bin, &f_fftw_output[0][0] + f_fft_size*2, &freq_data_out->get_array()[0][0]);
                        freq_data_out->set_pkt_in_batch(time_data_in->get_pkt_in_batch());
                        freq_data_out->set_pkt_in_session(time_data_in->get_pkt_in_session());

                        if ( f_enable_time_output && !out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "frequency_transform error setting time output stream to s_run" );
                            break;
                        }
                        if ( !out_stream< 1 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "frequency_transform error setting frequency output stream to s_run" );
                            break;
                        }
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
            bool t_t_stop_ok = f_enable_time_output && out_stream< 0 >().set( stream::s_stop );
            bool t_f_stop_ok = out_stream< 1 >().set( stream::s_stop );
            if( ! t_t_stop_ok && ! t_f_stop_ok) return;

            LDEBUG( plog, "Exiting output streams" );
            out_stream< 0 >().set( stream::s_exit );
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
        a_node->set_fft_size( a_config.get_value( "fft-size", a_node->get_fft_size() ) );
        a_node->set_transform_flag( a_config.get_value( "transform-flag", a_node->get_transform_flag() ) );
        a_node->set_use_wisdom( a_config.get_value( "use-wisdom", a_node->get_use_wisdom() ) );
        a_node->set_wisdom_filename( a_config.get_value( "wisdom-filename", a_node->get_wisdom_filename() ) );
        return;
    }

    void frequency_transform_binding::do_dump_config( const frequency_transform* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping frequency_transform configuration" );
        a_config.add( "time-length", scarab::param_value( a_node->get_time_length() ) );
        a_config.add( "freq-length", scarab::param_value( a_node->get_freq_length() ) );
        a_config.add( "fft-size", scarab::param_value( a_node->get_fft_size() ) );
        a_config.add( "transform-flag", scarab::param_value( a_node->get_transform_flag() ) );
        a_config.add( "use-wisdom", scarab::param_value( a_node->get_use_wisdom() ) );
        a_config.add( "wisdom-filename", scarab::param_value( a_node->get_wisdom_filename() ) );
        return;
    }

    bool frequency_transform_binding::do_run_command( frequency_transform* a_node, const std::string& a_cmd, const scarab::param_node& ) const
    {
        if ( a_cmd == "freq-only" )
        {
            LDEBUG( plog, "should enable freq-only mode" );
            a_node->switch_to_freq_only();
            return true;
        }
        else if ( a_cmd == "time-and-freq" )
        {
            LDEBUG( plog, "should enable time-and-freq mode" );
            a_node->switch_to_time_and_freq();
            return true;
        }
        else
        {
            LWARN( plog, "unrecognized command: <" << a_cmd << ">" );
            return false;
        }
    }

} /* namespace psyllid */
