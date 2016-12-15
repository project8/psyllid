/*
 * streaming_writer.cc
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#include "streaming_writer.hh"

#include "daq_control.hh"
#include "butterfly_house.hh"
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
    REGISTER_NODE_AND_BUILDER( streaming_writer, "streaming-writer" );

    LOGGER( plog, "streaming_writer" );

    streaming_writer::streaming_writer() :
            control_access(),
            f_file_size_limit_mb(),
            f_filename( "default_filename_strw.egg" ),
            f_description( "A very nice run" ),
            f_last_pkt_in_batch( 0 )
    {
    }

    streaming_writer::~streaming_writer()
    {
    }

    void streaming_writer::initialize()
    {
        return;
    }

    void streaming_writer::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_time_command = stream::s_none;

            time_data* t_time_data = nullptr;

            butterfly_house* t_bf_house = butterfly_house::get_instance();
            monarch_wrap_ptr t_monarch_ptr;
            stream_wrap_ptr t_swrap_ptr;
            monarch3::M3Record* t_record_ptr = nullptr;

            // TODO: make these parameters configurable
            uint64_t t_bit_depth = 8;
            uint64_t t_data_type_size = 1;
            uint64_t t_sample_size = 2;
            uint64_t t_rec_length = PAYLOAD_SIZE / t_sample_size;
            uint64_t t_bytes_per_record = t_rec_length * t_sample_size * t_data_type_size;
            uint64_t t_acq_rate = 100; // MHz
            scarab::time_nsec_type t_record_length_nsec = llrint( (double)(PAYLOAD_SIZE / 2) / (double)t_acq_rate * 1.e3 );
            double t_v_offset = 0.;
            double t_v_range = 0.5;

            scarab::dig_calib_params t_dig_params;
            scarab::get_calib_params( t_bit_depth, t_data_type_size, t_v_offset, t_v_range, true, &t_dig_params );

            unsigned t_stream_no = 0;
            monarch_time_point_t t_run_start_time;
            uint64_t t_first_pkt_in_run = 0;

            bool t_is_new_event = true;

            while( true )
            {
                t_time_command = in_stream< 0 >().get();
                LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );

                if( t_time_command == stream::s_exit )
                {
                    LDEBUG( plog, "Streaming writer is exiting" );

                    if( t_swrap_ptr )
                    {
                        LDEBUG( plog, "Finishing stream <" << t_stream_no << ">" );
                        t_monarch_ptr->finish_stream( t_stream_no, true );
                        t_swrap_ptr.reset();
                    }

                    break;
                }

                if( t_time_command == stream::s_stop )
                {
                    LDEBUG( plog, "Streaming writer is stopping" );

                    if( t_swrap_ptr )
                    {
                        LDEBUG( plog, "Finishing stream <" << t_stream_no << ">" );
                        t_monarch_ptr->finish_stream( t_stream_no, true );
                        t_swrap_ptr.reset();
                    }

                    continue;
                }

                t_time_data = in_stream< 0 >().data();

                if( t_time_command == stream::s_start )
                {
                    LDEBUG( plog, "Preparing egg file" );

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
                                monarch3::sDigitizedS, t_bit_depth, monarch3::sBitsAlignedLeft, &t_chan_vec );

                        //unsigned i_chan_psyllid = 0; // this is the channel number in mantis, as opposed to the channel number in the monarch file
                        for( std::vector< unsigned >::const_iterator it = t_chan_vec.begin(); it != t_chan_vec.end(); ++it )
                        {
                            t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetVoltageOffset( t_dig_params.v_offset );
                            t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetVoltageRange( t_dig_params.v_range );
                            t_hwrap_ptr->header().GetChannelHeaders()[ *it ].SetDACGain( t_dig_params.dac_gain );
                            //++i_chan_psyllid;
                        }

                        t_run_start_time = t_monarch_ptr->get_run_start_time();
                        t_first_pkt_in_run = t_time_data->get_pkt_in_session();

                        t_is_new_event = true;

                    }
                    catch( error& e )
                    {
                        throw midge::error() << "Unable to prepare the egg file <" << f_filename << ">: " << e.what();
                    }

                    continue;
                }

                if( t_time_command == stream::s_run )
                {
                    if( ! t_swrap_ptr )
                    {
                        LDEBUG( plog, "Getting stream <" << t_stream_no << ">" );
                        t_swrap_ptr = t_monarch_ptr->get_stream( t_stream_no );
                        t_record_ptr = t_swrap_ptr->get_stream_record();
                    }

                    uint64_t t_time_id = t_time_data->get_pkt_in_session();

                    uint32_t t_expected_pkt_in_batch = f_last_pkt_in_batch + 1;
                    if( t_expected_pkt_in_batch >= BATCH_COUNTER_SIZE ) t_expected_pkt_in_batch = 0;
                    if( ! t_is_new_event && t_time_data->get_pkt_in_batch() != t_expected_pkt_in_batch ) t_is_new_event = true;
                    f_last_pkt_in_batch = t_time_data->get_pkt_in_batch();

                    t_record_ptr->SetRecordId( t_time_id );
                    t_record_ptr->SetTime( t_record_length_nsec * ( t_time_id - t_first_pkt_in_run ) );
                    ::memcpy( t_record_ptr->GetData(), t_time_data->get_raw_array(), t_bytes_per_record );
                    t_swrap_ptr->write_record( t_is_new_event );

                    t_is_new_event = false;

                    continue;
                }

            }

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void streaming_writer::finalize()
    {
        return;
    }


    streaming_writer_builder::streaming_writer_builder() :
            _node_builder< streaming_writer >()
    {
    }

    streaming_writer_builder::~streaming_writer_builder()
    {
    }

    void streaming_writer_builder::apply_config( streaming_writer* a_node, const scarab::param_node& a_config )
    {
        a_node->set_file_size_limit_mb( a_config.get_value( "file-size-limit-mb", a_node->get_file_size_limit_mb() ) );
        return;
    }



} /* namespace psyllid */
