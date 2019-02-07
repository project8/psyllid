/*
 * streaming_writer.cc
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#include "streaming_writer.hh"

#include "butterfly_house.hh"
#include "psyllid_error.hh"

#include "midge_error.hh"

#include "digital.hh"
#include "logger.hh"
#include "time.hh"

#include <cmath>

using midge::stream;

using std::string;
using std::vector;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( streaming_writer, "streaming-writer", streaming_writer_binding );

    LOGGER( plog, "streaming_writer" );

    streaming_writer::streaming_writer() :
            egg_writer(),
            f_file_num( 0 ),
            f_bit_depth( 8 ),
            f_data_type_size( 1 ),
            f_sample_size( 2 ),
            f_record_size( 4096 ),
            f_acq_rate( 100 ),
            f_v_offset( 0. ),
            f_v_range( 0.5 ),
            f_center_freq( 50.e6 ),
            f_freq_range( 100.e6 ),
            f_last_pkt_in_batch( 0 ),
            f_monarch_ptr(),
            f_stream_no( 0 )
    {
    }

    streaming_writer::~streaming_writer()
    {
    }

    void streaming_writer::prepare_to_write( monarch_wrap_ptr a_mw_ptr, header_wrap_ptr a_hw_ptr )
    {
        f_monarch_ptr = a_mw_ptr;

        scarab::dig_calib_params t_dig_params;
        scarab::get_calib_params( f_bit_depth, f_data_type_size, f_v_offset, f_v_range, true, &t_dig_params );

        vector< unsigned > t_chan_vec;
        f_stream_no = a_hw_ptr->header().AddStream( "Psyllid - ROACH2",
                f_acq_rate, f_record_size, f_sample_size, f_data_type_size,
                monarch3::sDigitizedS, f_bit_depth, monarch3::sBitsAlignedLeft, &t_chan_vec );

        //unsigned i_chan_psyllid = 0; // this is the channel number in psyllid, as opposed to the channel number in the monarch file
        for( std::vector< unsigned >::const_iterator it = t_chan_vec.begin(); it != t_chan_vec.end(); ++it )
        {
            a_hw_ptr->header().GetChannelHeaders()[ *it ].SetVoltageOffset( t_dig_params.v_offset );
            a_hw_ptr->header().GetChannelHeaders()[ *it ].SetVoltageRange( t_dig_params.v_range );
            a_hw_ptr->header().GetChannelHeaders()[ *it ].SetDACGain( t_dig_params.dac_gain );
            a_hw_ptr->header().GetChannelHeaders()[ *it ].SetFrequencyMin( f_center_freq - 0.5 * f_freq_range );
            a_hw_ptr->header().GetChannelHeaders()[ *it ].SetFrequencyRange( f_freq_range );

            //++i_chan_psyllid;
        }

        return;
    }

    void streaming_writer::initialize()
    {
        butterfly_house::get_instance()->register_writer( this, f_file_num );
        return;
    }

    void streaming_writer::execute( midge::diptera* a_midge )
    {
        LDEBUG( plog, "execute streaming writer" );
        try
        {
            midge::enum_t t_time_command = stream::s_none;

            time_data* t_time_data = nullptr;

            stream_wrap_ptr t_swrap_ptr;

            uint64_t t_bytes_per_record = f_record_size * f_sample_size * f_data_type_size;
            uint64_t t_record_length_nsec = llrint( (double)(PAYLOAD_SIZE / 2) / (double)f_acq_rate * 1.e3 );

            uint64_t t_first_pkt_in_run = 0;

            bool t_is_new_acquisition = true;
            bool t_start_file_with_next_data = false;

            while( ! is_canceled() )
            {
                t_time_command = in_stream< 0 >().get();
                if( t_time_command == stream::s_none ) continue;
                if( t_time_command == stream::s_error ) break;

                LTRACE( plog, "Egg writer reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );

                if( t_time_command == stream::s_exit )
                {
                    LDEBUG( plog, "Streaming writer is exiting" );

                    if( t_swrap_ptr )
                    {
                        f_monarch_ptr->finish_stream( f_stream_no );
                        t_swrap_ptr.reset();
                    }

                    break;
                }

                if( t_time_command == stream::s_stop )
                {
                    LDEBUG( plog, "Streaming writer is stopping" );

                    if( t_swrap_ptr )
                    {
                        f_monarch_ptr->finish_stream( f_stream_no );
                        t_swrap_ptr.reset();
                    }

                    continue;
                }

                if( t_time_command == stream::s_start )
                {
                    LDEBUG( plog, "Will start file with next data" );

                    if( t_swrap_ptr ) t_swrap_ptr.reset();

                    LDEBUG( plog, "Getting stream <" << f_stream_no << ">" );
                    t_swrap_ptr = f_monarch_ptr->get_stream( f_stream_no );

                    t_start_file_with_next_data = true;
                    continue;
                }

                if( t_time_command == stream::s_run )
                {
                    t_time_data = in_stream< 0 >().data();

                    if( t_start_file_with_next_data )
                    {
                        LDEBUG( plog, "Handling first packet in run" );

                        t_first_pkt_in_run = t_time_data->get_pkt_in_session();

                        t_is_new_acquisition = true;

                        t_start_file_with_next_data = false;
                    }

                    uint64_t t_time_id = t_time_data->get_pkt_in_session();
                    LTRACE( plog, "Writing packet (in session) " << t_time_id );

                    uint32_t t_expected_pkt_in_batch = f_last_pkt_in_batch + 1;
                    if( t_expected_pkt_in_batch >= BATCH_COUNTER_SIZE ) t_expected_pkt_in_batch = 0;
                    if( ! t_is_new_acquisition && t_time_data->get_pkt_in_batch() != t_expected_pkt_in_batch ) t_is_new_acquisition = true;
                    f_last_pkt_in_batch = t_time_data->get_pkt_in_batch();

                    if( ! t_swrap_ptr->write_record( t_time_id, t_record_length_nsec * ( t_time_id - t_first_pkt_in_run ), t_time_data->get_raw_array(), t_bytes_per_record, t_is_new_acquisition ) )
                    {
                        throw midge::node_nonfatal_error() << "Unable to write record to file; record ID: " << t_time_id;
                    }

                    LTRACE( plog, "Packet written (" << t_time_id << ")" );

                    t_is_new_acquisition = false;

                    continue;
                }

            } // end while( ! is_cancelled() )

            // final attempt to finish the stream if the outer while loop is broken without the stream having been stopped or exited
            // e.g. if cancelled first, before anything else happens
            if( t_swrap_ptr )
            {
                f_monarch_ptr->finish_stream( f_stream_no );
                t_swrap_ptr.reset();
            }

            return;
        }
        catch(...)
        {
            LWARN( plog, "an error occurred executing streaming writer" );
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void streaming_writer::finalize()
    {
        LDEBUG( plog, "finalize streaming writer" );
        butterfly_house::get_instance()->unregister_writer( this );
        return;
    }


    streaming_writer_binding::streaming_writer_binding() :
            _node_binding< streaming_writer, streaming_writer_binding >()
    {
    }

    streaming_writer_binding::~streaming_writer_binding()
    {
    }

    void streaming_writer_binding::do_apply_config( streaming_writer* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring streaming_writer with:\n" << a_config );
        a_node->set_file_num( a_config.get_value( "file-num", a_node->get_file_num() ) );
        if( a_config.has( "device" ) )
        {
            const scarab::param_node& t_dev_config = a_config["device"].as_node();
            a_node->set_bit_depth( t_dev_config.get_value( "bit-depth", a_node->get_bit_depth() ) );
            a_node->set_data_type_size( t_dev_config.get_value( "data-type-size", a_node->get_data_type_size() ) );
            a_node->set_sample_size( t_dev_config.get_value( "sample-size", a_node->get_sample_size() ) );
            a_node->set_record_size( t_dev_config.get_value( "record-size", a_node->get_record_size() ) );
            a_node->set_acq_rate( t_dev_config.get_value( "acq-rate", a_node->get_acq_rate() ) );
            a_node->set_v_offset( t_dev_config.get_value( "v-offset", a_node->get_v_offset() ) );
            a_node->set_v_range( t_dev_config.get_value( "v-range", a_node->get_v_range() ) );
        }
        a_node->set_center_freq( a_config.get_value( "center-freq", a_node->get_center_freq() ) );
        a_node->set_freq_range( a_config.get_value( "freq-range", a_node->get_freq_range() ) );
        return;
    }

    void streaming_writer_binding::do_dump_config( const streaming_writer* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for streaming_writer" );
        a_config.add( "file-num", a_node->get_file_num() );
        scarab::param_node t_dev_node = scarab::param_node();
        t_dev_node.add( "bit-depth", a_node->get_bit_depth() );
        t_dev_node.add( "data-type-size", a_node->get_data_type_size() );
        t_dev_node.add( "sample-size", a_node->get_sample_size() );
        t_dev_node.add( "record-size", a_node->get_record_size() );
        t_dev_node.add( "acq-rate", a_node->get_acq_rate() );
        t_dev_node.add( "v-offset", a_node->get_v_offset() );
        t_dev_node.add( "v-range", a_node->get_v_range() );
        a_config.add( "device", t_dev_node );
        a_config.add( "center-freq", a_node->get_center_freq() );
        a_config.add( "freq-range", a_node->get_freq_range() );
        return;
    }


} /* namespace psyllid */
