/*
 * triggered_writer.cc
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#include "triggered_writer.hh"

#include "daq_control.hh"
#include "psyllid_error.hh"

#include "digital.hh"
#include "logger.hh"
#include "time.hh"

#include <cmath>

using midge::stream;

using std::string;
using std::vector;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( triggered_writer, "triggered-writer", triggered_writer_binding );

    LOGGER( plog, "triggered_writer" );

    triggered_writer::triggered_writer() :
            control_access(),
            egg_writer(),
            f_file_num( 0 ),
            f_file_size_limit_mb( 2000 ),
            f_filename( "default_filename_tw.egg" ),
            f_description( "A very nice run" ),
            f_bit_depth( 8 ),
            f_data_type_size( 1 ),
            f_sample_size( 2 ),
            f_record_size( 4096 ),
            f_acq_rate( 100 ),
            f_v_offset( 0. ),
            f_v_range( 0.5 ),
            f_center_freq( 50.e6 ),
            f_freq_range( 100.e6 )
    {
    }

    triggered_writer::~triggered_writer()
    {
    }

    void triggered_writer::initialize()
    {
        butterfly_house::get_instance()->register_writer( this, f_file_num );
        return;
    }

    void triggered_writer::execute( midge::diptera* a_midge )
    {
        try
        {
            exe_loop_context t_ctx;
            t_ctx.f_is_running = false;
            t_ctx.f_should_exit = false;
            t_ctx.f_record_ptr = nullptr;
            t_ctx.f_stream_no = 0;
            t_ctx.f_first_pkt_in_run = 0;
            t_ctx.f_is_new_event = true;

            // outer while loop to switch between the two exe loops until canceled
            while( ! is_canceled() && ! t_ctx.f_should_exit )
            {
                if( t_ctx.f_is_running )
                {
                    exe_loop_is_running( t_ctx );
                }
                else
                {
                    exe_loop_not_running( t_ctx );
                }
            } // end while ! is_canceled()

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void egg_writer::exe_loop_not_running( exe_loop_context& a_ctx )
    {
        midge::enum_t t_trig_command = stream::s_none;
        midge::enum_t t_time_command = stream::s_none;

        butterfly_house* t_bf_house = butterfly_house::get_instance();

        time_data* t_time_data = nullptr;

        while( ! is_canceled() )
        {
            t_time_command = in_stream< 0 >().get();
            if( t_time_command == stream::s_none ) continue;
            if( t_time_command == stream::s_error )
            {
                a_ctx.f_should_exit = true;
                break;
            }

            LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );

            if( t_time_command == stream::s_exit )
            {
                LDEBUG( plog, "Egg writer is exiting due to time-stream command; no run in progress" );
                a_ctx.f_should_exit = true;
                break;
            }

            if( t_time_command == stream::s_stop )
            {
                LDEBUG( plog, "Egg writer received stop command on the time stream while already stopped; no action taken" );
                continue;
            }

            if( t_time_command == stream::s_run )
            {
                LWARN( plog, "Egg writer received run command on the time stream while stopped; no action taken" );
                continue;
            }

            if( t_time_command == stream::s_start )
            {
                LDEBUG( plog, "Egg writer received start command on the time stream; looking for start command on the trigger stream" );

                // do this in a while loop so we don't re-do the time stream get()
                while( ! is_canceled() )
                {
                    t_trig_command = stream::s_none;
                    for( unsigned i_attempt = 0; i_attempt < 10 && t_trig_command != stream::s_start; ++i_attempt )
                    {
                        t_trig_command = in_stream< 1 >().get();
                        LTRACE( plog, "(attempt " << i_attempt << ") Egg writer reading stream 1 (trig) at index " << in_stream< 1 >().get_current_index() );
                    }

                    if( t_trig_command == stream::s_start )
                    {
                        LDEBUG( plog, "Time and trig commands match: start" );
                        LINFO( plog, "Starting a run: preparing egg file" );

                        try
                        {
                            unsigned t_run_duration = 0;
                            if( ! f_daq_control.expired() )
                            {
                                std::shared_ptr< daq_control > t_daq_control = f_daq_control.lock();
                                f_filename = t_daq_control->run_filename();
                                f_description = t_daq_control->run_description();
                                t_run_duration = t_daq_control->get_run_duration();
                                LDEBUG( plog, "Updated filename, description, and duration from daq_control" );
                            }

                            LDEBUG( plog, "Declaring monarch file <" << f_filename << ">" );
                            a_ctx.f_monarch_ptr = t_bf_house->declare_file( f_filename );
                            header_wrap_ptr t_hwrap_ptr = a_ctx.f_monarch_ptr->get_header();

                            if( ! t_hwrap_ptr->global_setup_done() )
                            {
                                t_time_data = in_stream< 0 >().data();

                                t_hwrap_ptr->header().SetDescription( f_description );

                                time_t t_raw_time = t_time_data->get_unix_time();
                                struct tm* t_processed_time = gmtime( &t_raw_time );
                                char t_timestamp[ 512 ];
                                strftime( t_timestamp, 512, scarab::date_time_format, t_processed_time );
                                t_hwrap_ptr->header().SetTimestamp( t_timestamp );

                                t_hwrap_ptr->header().SetRunDuration( t_run_duration );
                                t_hwrap_ptr->global_setup_done( true );
                            }

                            vector< unsigned > t_chan_vec;
                            a_ctx.f_stream_no = t_hwrap_ptr->header().AddStream( "Psyllid - ROACH2",
                                    f_acq_rate, f_record_size, f_sample_size, f_data_type_size,
                                    monarch3::sDigitizedS, f_bit_depth, monarch3::sBitsAlignedLeft, &t_chan_vec );

                            scarab::dig_calib_params t_dig_params;
                            scarab::get_calib_params( f_bit_depth, f_data_type_size, f_v_offset, f_v_range, true, &t_dig_params );

                            //unsigned i_chan_psyllid = 0; // this is the channel number in mantis, as opposed to the channel number in the monarch file
                            for( std::vector< unsigned >::const_iterator it = t_chan_vec.begin(); it != t_chan_vec.end(); ++it )
                            {
                                t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetVoltageOffset( t_dig_params.v_offset );
                                t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetVoltageRange( t_dig_params.v_range );
                                t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetDACGain( t_dig_params.dac_gain );
                                t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetFrequencyMin( f_center_freq - 0.5 * f_freq_range );
                                t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetFrequencyRange( f_freq_range );
                                //++i_chan_psyllid;
                            }

                            //t_run_start_time = a_ctx.f_monarch_ptr->get_run_start_time();
                            a_ctx.f_first_pkt_in_run = t_time_data->get_pkt_in_session();

                            a_ctx.f_is_new_event = true;
                        }
                        catch( error& e )
                        {
                            throw error() << "Unable to prepare the egg file <" << f_filename << ">: " << e.what();
                        }

                        break; // break out of while loop looking for start trig command
                    }
                    else
                    {
                        throw error() << "Egg writer received unexpected trig-stream command while waiting for start: " << t_trig_command;
                    } // end if-else block for start trig command

                } // end while loop looking for start trig command

                // should end up here after finding the start trig command and setting up the monarch stream
                LDEBUG( plog, "Breaking out of not-running exe loop" );
                a_ctx.f_is_running = true;
                break; // break out of not-running exe loop
            } // end if time command is start

        } // end while ! is_canceled()

        return;
    }

    void egg_writer::exe_loop_is_running( exe_loop_context& a_ctx )
    {
        midge::enum_t t_trig_command = stream::s_none;
        midge::enum_t t_time_command = stream::s_none;

        trigger_flag* t_trig_data = nullptr;
        time_data* t_time_data = nullptr;

        uint64_t t_bytes_per_record = f_record_size * f_sample_size * f_data_type_size;
        scarab::time_nsec_type t_record_length_nsec = llrint( (double)(PAYLOAD_SIZE / 2) / (double)f_acq_rate * 1.e3 );

        while( ! is_canceled() )
        {
            t_trig_command = in_stream< 1 >().get();
            LTRACE( plog, "Egg writer reading stream 1 (trig) at index " << in_stream< 1 >().get_current_index() );

            if( t_trig_command == stream::s_none )
            {
                LDEBUG( plog, "Egg writer received none command on the trig stream while run is in progress");
                t_time_command = in_stream< 0 >().get();
                LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );
                LDEBUG( plog, "Advancing time stream; time command matched trig command? trig command = " << t_trig_command << "; time command = " << t_time_command );
                if( t_time_command != stream::s_none )
                {
                    if( a_ctx.f_swrap_ptr )
                    {
                        LDEBUG( plog, "Finishing stream <" << a_ctx.f_stream_no << ">" );
                        a_ctx.f_monarch_ptr->finish_stream( a_ctx.f_stream_no, true );
                        a_ctx.f_swrap_ptr.reset();
                    }
                    throw error() << "Trig command doesn't match time command: time command = " << t_time_command << "; trig command = " << t_trig_command;
                }
                continue;
            }
            if( t_trig_command == stream::s_error )
            {
                a_ctx.f_should_exit = true;
                break;
            }

            if( t_trig_command == stream::s_exit )
            {
                LDEBUG( plog, "Egg writer is exiting due to trig-stream command; run is in progress" );
                if( a_ctx.f_swrap_ptr )
                {
                    LDEBUG( plog, "Finishing stream <" << a_ctx.f_stream_no << ">" );
                    a_ctx.f_monarch_ptr->finish_stream( a_ctx.f_stream_no, true );
                    a_ctx.f_swrap_ptr.reset();
                }
                a_ctx.f_should_exit = true;
                break;
            }

            if( t_trig_command == stream::s_stop )
            {
                LDEBUG( plog, "Egg writer received stop command on the trig stream while run is in progress" );
                if( a_ctx.f_swrap_ptr )
                {
                    LDEBUG( plog, "Finishing stream <" << a_ctx.f_stream_no << ">" );
                    a_ctx.f_monarch_ptr->finish_stream( a_ctx.f_stream_no, true );
                    a_ctx.f_swrap_ptr.reset();
                }
                t_time_command = in_stream< 0 >().get();
                LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );
                LDEBUG( plog, "Advancing time stream; time command matched trig command? trig command = " << t_trig_command << "; time command = " << t_time_command );
                LDEBUG( plog, "Breaking out of is-running exe loop" );
                a_ctx.f_is_running = false;
                break; // out of is-running exe loop
            }

            if( t_trig_command == stream::s_start )
            {
                LERROR( plog, "Egg writer received unexpected start command on the trig stream while running")
                if( a_ctx.f_swrap_ptr )
                {
                    LDEBUG( plog, "Finishing stream <" << a_ctx.f_stream_no << ">" );
                    a_ctx.f_monarch_ptr->finish_stream( a_ctx.f_stream_no, true );
                    a_ctx.f_swrap_ptr.reset();
                }
                t_time_command = in_stream< 0 >().get();
                LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );
                LDEBUG( plog, "Advancing time stream; time command matched trig command? trig command = " << t_trig_command << "; time command = " << t_time_command );
                throw error() << "Egg writer received unexpected start command on the trig stream while running";
            }

            if( t_trig_command == stream::s_run )
            {
                t_time_command = in_stream< 0 >().get();
                LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );
                LTRACE( plog, "Advancing time stream; time command matched trig command? trig command = " << t_trig_command << "; time command = " << t_time_command );
                if( t_time_command != stream::s_run )
                {
                    if( a_ctx.f_swrap_ptr )
                    {
                        LDEBUG( plog, "Finishing stream <" << a_ctx.f_stream_no << ">" );
                        a_ctx.f_monarch_ptr->finish_stream( a_ctx.f_stream_no, true );
                        a_ctx.f_swrap_ptr.reset();
                    }
                    throw error() << "Trig command doesn't match time command: time command = " << t_time_command << "; trig command = " << t_trig_command;
                }

                // everything agrees that we're running

                t_time_data = in_stream< 0 >().data();
                t_trig_data = in_stream< 1 >().data();

                if( ! a_ctx.f_swrap_ptr )
                {
                    LDEBUG( plog, "Getting stream <" << a_ctx.f_stream_no << ">" );
                    a_ctx.f_swrap_ptr = a_ctx.f_monarch_ptr->get_stream( a_ctx.f_stream_no );
                    a_ctx.f_record_ptr = a_ctx.f_swrap_ptr->get_stream_record();
                }

                uint64_t t_time_id = t_time_data->get_pkt_in_session();
                uint64_t t_trig_id = t_trig_data->get_id();

                if( t_time_id != t_trig_id )
                {
                    LTRACE( plog, "Mismatch between time id <" << t_time_id << "> and trigger id <" << t_trig_id << ">" );
                    while( t_time_id < t_trig_id )
                    {
                        LDEBUG( plog, "Moving time stream forward" );
                        t_time_command = in_stream< 0 >().get();
                        t_time_data = in_stream< 0 >().data();
                        t_time_id = t_time_data->get_pkt_in_session();
                    }
                    while( t_time_id > t_trig_id )
                    {
                        LTRACE( plog, "Moving trig stream forward" );
                        t_trig_command = in_stream< 1 >().get();
                        t_trig_data = in_stream< 1 >().data();
                        t_trig_id = t_trig_data->get_id();
                    }
                    if( t_time_id != t_trig_id )
                    {
                        throw midge::error() << "Unable to match time and trigger streams";
                    }
                    LTRACE( plog, "Mismatch resolved: time id <" << t_time_id << "> and trigger id <" << t_trig_id << ">" );
                }

                if( t_trig_data->get_flag() )
                {
                    LTRACE( plog, "Triggered packet, id <" << t_trig_data->get_id() << ">" );

                    if( a_ctx.f_is_new_event ) LDEBUG( plog, "New event" );
                    a_ctx.f_record_ptr->SetRecordId( t_time_id );
                    a_ctx.f_record_ptr->SetTime( t_record_length_nsec * ( t_time_id - a_ctx.f_first_pkt_in_run ) );
                    memcpy( a_ctx.f_record_ptr->GetData(), t_time_data->get_raw_array(), t_bytes_per_record );
                    a_ctx.f_swrap_ptr->write_record( a_ctx.f_is_new_event );
                    a_ctx.f_is_new_event = false;
                }
                else
                {
                    LTRACE( plog, "Untriggered packet, id <" << t_trig_id << ">" );
                    a_ctx.f_is_new_event = true;
                }

                continue;
            }

        } // end while ! is_canceled()

        return;
    }

    void triggered_writer::finalize()
    {
        butterfly_house::get_instance()->unregister_writer( this );
        return;
    }


    triggered_writer_binding::triggered_writer_binding() :
            _node_binding< triggered_writer, triggered_writer_binding >()
    {
    }

    triggered_writer_binding::~triggered_writer_binding()
    {
    }

    void triggered_writer_binding::do_apply_config( triggered_writer* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring triggered_writer with:\n" << a_config );
        a_node->set_file_size_limit_mb( a_config.get_value( "file-size-limit-mb", a_node->get_file_size_limit_mb() ) );
        const scarab::param_node *t_dev_config = a_config.node_at( "device" );
        if( t_dev_config != nullptr )
        {
            a_node->set_bit_depth( t_dev_config->get_value( "bit-depth", a_node->get_bit_depth() ) );
            a_node->set_data_type_size( t_dev_config->get_value( "data-type-size", a_node->get_data_type_size() ) );
            a_node->set_sample_size( t_dev_config->get_value( "sample-size", a_node->get_sample_size() ) );
            a_node->set_record_size( t_dev_config->get_value( "record-size", a_node->get_record_size() ) );
            a_node->set_acq_rate( t_dev_config->get_value( "acq-rate", a_node->get_acq_rate() ) );
            a_node->set_v_offset( t_dev_config->get_value( "v-offset", a_node->get_v_offset() ) );
            a_node->set_v_range( t_dev_config->get_value( "v-range", a_node->get_v_range() ) );
        }
        a_node->set_center_freq( a_config.get_value( "center-freq", a_node->get_center_freq() ) );
        a_node->set_freq_range( a_config.get_value( "freq-range", a_node->get_freq_range() ) );
        return;
    }

    void triggered_writer_binding::do_dump_config( const triggered_writer* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for triggered_writer" );
        a_config.add( "file-size-limit-mb", new scarab::param_value( a_node->get_file_size_limit_mb() ) );
        scarab::param_node* t_dev_node = new scarab::param_node();
        t_dev_node->add( "bit-depth", new scarab::param_value( a_node->get_bit_depth() ) );
        t_dev_node->add( "data-type-size", new scarab::param_value( a_node->get_data_type_size() ) );
        t_dev_node->add( "sample-size", new scarab::param_value( a_node->get_sample_size() ) );
        t_dev_node->add( "record-size", new scarab::param_value( a_node->get_record_size() ) );
        t_dev_node->add( "acq-rate", new scarab::param_value( a_node->get_acq_rate() ) );
        t_dev_node->add( "v-offset", new scarab::param_value( a_node->get_v_offset() ) );
        t_dev_node->add( "v-range", new scarab::param_value( a_node->get_v_range() ) );
        a_config.add( "device", t_dev_node );
        a_config.add( "center-freq", new scarab::param_value( a_node->get_center_freq() ) );
        a_config.add( "freq-range", new scarab::param_value( a_node->get_freq_range() ) );
        return;
    }

} /* namespace psyllid */
