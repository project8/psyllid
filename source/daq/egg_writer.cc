/*
 * egg_writer.cc
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#include "egg_writer.hh"

#include "daq_control.hh"
#include "butterfly_house.hh"
#include "psyllid_error.hh"

#include "logger.hh"
#include "time.hh"

#include <cmath>

using midge::stream;

using std::string;
using std::vector;

namespace psyllid
{
    REGISTER_NODE( egg_writer, "egg-writer" );

    LOGGER( plog, "egg_writer" );

    egg_writer::egg_writer() :
            control_access(),
            f_file_size_limit_mb(),
            f_filename( "default_filename_ew.egg" ),
            f_description( "A very nice run" )
    {
    }

    egg_writer::~egg_writer()
    {
    }

    void egg_writer::initialize()
    {
        return;
    }

    void egg_writer::execute()
    {
        midge::enum_t t_trig_command = stream::s_none;
        midge::enum_t t_time_command = stream::s_none;

        trigger_flag* t_trig_data = nullptr;
        time_data* t_time_data = nullptr;

        butterfly_house* t_bf_house = butterfly_house::get_instance();
        monarch_wrap_ptr t_monarch_ptr;
        stream_wrap_ptr t_swrap_ptr;
        monarch3::M3Record* t_record_ptr = nullptr;

        uint64_t t_bit_depth = 8;
        uint64_t t_data_type_size = 1;
        uint64_t t_sample_size = 2;
        uint64_t t_rec_length = PAYLOAD_SIZE / t_sample_size;
        uint64_t t_bytes_per_record = t_rec_length * t_sample_size * t_data_type_size;
        uint64_t t_acq_rate = 100; // MHz
        scarab::time_nsec_type t_record_length_nsec = llrint( (double)(PAYLOAD_SIZE / 2) / (double)t_acq_rate * 1.e3 );

        unsigned t_stream_no = 0;
        monarch_time_point_t t_run_start_time;
        uint64_t t_first_pkt_in_run = 0;

        bool t_is_new_event = true;
        //bool t_was_triggered = false;

        while( true )
        {
            t_time_command = in_stream< 0 >().get();
            DEBUG( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );

            t_trig_command = in_stream< 1 >().get();
            DEBUG( plog, "Egg writer reading stream 1 (trig) at index " << in_stream< 1 >().get_current_index() );
/*
            if( t_time_command != t_trig_command )
            {
                throw midge::error() << "Time and event stream commands are mismatched: " <<
                        t_time_command << " (time, at stream index " << in_stream< 0 >().get_current_index() << ") vs " <<
                        t_trig_command << " (trigger, at stream index " << in_stream< 1 >().get_current_index() << ")";
            }
*/
            if( t_trig_command == stream::s_exit || t_time_command == stream::s_exit )
            {
                DEBUG( plog, "Egg writer is exiting" );

                if( t_swrap_ptr )
                {
                    DEBUG( plog, "Finishing stream <" << t_stream_no << ">" );
                    t_monarch_ptr->finish_stream( t_stream_no, true );
                    t_swrap_ptr.reset();
                }

                break;
            }

            if( t_trig_command == stream::s_stop || t_time_command == stream::s_stop )
            {
                DEBUG( plog, "Egg writer is stopping" );

                if( t_swrap_ptr )
                {
                    DEBUG( plog, "Finishing stream <" << t_stream_no << ">" );
                    t_monarch_ptr->finish_stream( t_stream_no, true );
                    t_swrap_ptr.reset();
                }

                continue;
            }

            t_time_data = in_stream< 0 >().data();
            t_trig_data = in_stream< 1 >().data();

            if( t_trig_command == stream::s_start )
            {
                DEBUG( plog, "Preparing egg file" );

                try
                {
                    unsigned t_run_duration = 0;
                    if( ! f_daq_control.expired() )
                    {
                        std::shared_ptr< daq_control > t_daq_control = f_daq_control.lock();
                        f_filename = t_daq_control->run_filename();
                        f_description = t_daq_control->run_description();
                        t_run_duration = t_daq_control->get_run_duration();
                    }

                    t_monarch_ptr = t_bf_house->declare_file( f_filename );
                    header_wrap_ptr t_hwrap_ptr = t_monarch_ptr->get_header();

                    if( ! t_hwrap_ptr->global_setup_done() )
                    {
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
                    t_stream_no = t_hwrap_ptr->header().AddStream( "Psyllid - ROACH2",
                            t_acq_rate, t_rec_length, t_sample_size, t_data_type_size,
                            monarch3::sDigitizedUS, t_bit_depth, monarch3::sBitsAlignedLeft, &t_chan_vec );

                    //unsigned i_chan_psyllid = 0; // this is the channel number in mantis, as opposed to the channel number in the monarch file
                    for( std::vector< unsigned >::const_iterator it = t_chan_vec.begin(); it != t_chan_vec.end(); ++it )
                    {
                        t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetVoltageOffset( 0. );
                        t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetVoltageRange( 0.5 );
                        t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetDACGain( 1. );
                        //++i_chan_psyllid;
                    }

                    t_run_start_time = t_monarch_ptr->get_run_start_time();
                    t_first_pkt_in_run = t_time_data->get_pkt_in_session();

                }
                catch( error& e )
                {
                    throw midge::error() << "Unable to prepare the egg file <" << f_filename << ">: " << e.what();
                }

                continue;
            }

            if( t_trig_command == stream::s_run )
            {
                if( ! t_swrap_ptr )
                {
                    DEBUG( plog, "Getting stream <" << t_stream_no << ">" );
                    t_swrap_ptr = t_monarch_ptr->get_stream( t_stream_no );
                    t_record_ptr = t_swrap_ptr->get_stream_record();
                }

                uint64_t t_time_id = t_time_data->get_pkt_in_session();
                uint64_t t_trig_id = t_trig_data->get_id();

                if( t_time_id != t_trig_id )
                {
                    DEBUG( plog, "Mismatch between time id <" << t_time_id << "> and trigger id <" << t_trig_id << ">" );
                    while( t_time_id < t_trig_id )
                    {
                        DEBUG( plog, "Moving time stream forward" );
                        t_time_command = in_stream< 0 >().get();
                        t_time_data = in_stream< 0 >().data();
                        t_time_id = t_time_data->get_pkt_in_session();
                    }
                    while( t_time_id > t_trig_id )
                    {
                        DEBUG( plog, "Moving trig stream forward" );
                        t_trig_command = in_stream< 1 >().get();
                        t_trig_data = in_stream< 1 >().data();
                        t_trig_id = t_trig_data->get_id();
                    }
                    if( t_time_id != t_trig_id )
                    {
                        throw midge::error() << "Unable to match time and trigger streams";
                    }
                    DEBUG( plog, "Mismatch resolved: time id <" << t_time_id << "> and trigger id <" << t_trig_id << ">" );
                }


                if( t_trig_data->get_flag() )
                {
                    DEBUG( plog, "Triggered packet, id <" << t_trig_data->get_id() << ">" );

                    if( t_is_new_event ) DEBUG( plog, "New event" );
                    t_record_ptr->SetRecordId( t_time_id );
                    t_record_ptr->SetTime( t_record_length_nsec * ( t_time_id - t_first_pkt_in_run ) );
                    memcpy( t_record_ptr->GetData(), t_time_data->get_raw_array(), t_bytes_per_record );
                    t_swrap_ptr->write_record( t_is_new_event );
                    t_is_new_event = false;
                }
                else
                {
                    DEBUG( plog, "Untriggered packet, id <" << t_trig_id << ">" );
                    t_is_new_event = true;
                }

                continue;
            }

        }

        return;
    }

    void egg_writer::finalize()
    {
        return;
    }


    egg_writer_builder::egg_writer_builder() :
            _node_builder< egg_writer >()
    {
    }

    egg_writer_builder::~egg_writer_builder()
    {
    }

    void egg_writer_builder::apply_config( egg_writer* a_node, const scarab::param_node& a_config )
    {
        a_node->set_file_size_limit_mb( a_config.get_value( "file-size-limit-mb", a_node->get_file_size_limit_mb() ) );
        return;
    }



} /* namespace psyllid */
