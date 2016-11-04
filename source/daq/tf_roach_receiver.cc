/*
 * udp_receiver.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "tf_roach_receiver.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <thread>
#include <memory>
#include <sys/types.h> // for ssize_t

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( tf_roach_receiver, "tf-roach-receiver" );

    LOGGER( plog, "tf_roach_receiver" );

    tf_roach_receiver::tf_roach_receiver() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            f_udp_buffer_size( sizeof( roach_packet ) ),
            f_time_sync_tol( 2 ),
            f_paused( false )
    {
    }

    tf_roach_receiver::~tf_roach_receiver()
    {
    }

    void tf_roach_receiver::initialize()
    {
        out_buffer< 0 >().initialize( f_time_length );
        out_buffer< 1 >().initialize( f_freq_length );
        return;
    }

    void tf_roach_receiver::execute()
    {
        LDEBUG( plog, "Executing the TF ROACH receiver" );

        midge::enum_t t_in_command = stream::s_none;

        memory_block* t_memory_block = nullptr;
        time_data* t_time_data = nullptr;
        freq_data* t_freq_data = nullptr;

        try
        {
            //LDEBUG( plog, "Server is listening" );

            std::unique_ptr< char[] > t_buffer_ptr( new char[ f_udp_buffer_size ] );

            //out_stream< 0 >().set( stream::s_start );
            //out_stream< 1 >().set( stream::s_start );
            f_paused = true;
            bool t_unpausing = false;

            uint32_t t_last_packet_time = 0;
            uint64_t t_time_session_pkt_counter = 0;
            uint64_t t_freq_session_pkt_counter = 0;

            uint64_t t_time_batch_pkt = 0;
            uint64_t t_freq_batch_pkt = 0;

            ssize_t t_size_received = 0;

            LINFO( plog, "Starting main loop; waiting for packets" );
            while( ! f_canceled.load() )
            {
                t_size_received = 0;

                check_instruction();

                t_in_command = in_stream< 0 >().get();
                LDEBUG( plog, "TF ROACH receiver reading stream 0 at index " << in_stream< 0 >().get_current_index() );

                if( t_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "TF ROACH receiver is exiting" );

                    break;
                }

                if( t_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "TF ROACH receiver is stopping" );
                    continue;
                }

                if( t_in_command == stream::s_start )
                {
                    LDEBUG( plog, "TF ROACH receiver is starting" );
                    continue;
                }

                if( f_canceled.load() )
                {
                    break;
                }
                check_instruction();

                // do nothing with input data if paused
                if( f_paused ) continue;

                if( t_in_command == stream::s_run )
                {
                    t_memory_block = in_stream< 0 >().data();





                    LDEBUG( plog, "Waiting for UDP packets" );

                    // inner loop over packet-receive timeouts
                    while( t_size_received <= 0 && ! f_canceled.load() )
                    {
                        //t_size_received = f_server->recv( t_buffer_ptr.get(), f_udp_buffer_size, 0 );
                        t_size_received = f_server->get_next_packet( t_buffer_ptr.get(), f_udp_buffer_size );
                    }

                    if( f_canceled.load() )
                    {
                        break;
                    }

                    if( t_size_received > 0 )
                    {
                        byteswap_inplace( reinterpret_cast< raw_roach_packet* >( t_buffer_ptr.get() ) );
                        roach_packet* t_roach_packet = reinterpret_cast< roach_packet* >( t_buffer_ptr.get() );

                        if( f_paused )
                        {
                            t_last_packet_time = t_roach_packet->f_unix_time;
                            continue;
                        }

                        // debug purposes only
    #ifndef NDEBUG
                        raw_roach_packet* t_raw_packet = reinterpret_cast< raw_roach_packet* >( t_buffer_ptr.get() );
                        LDEBUG( plog, "Raw packet header: " << std::hex << t_raw_packet->f_word_0 << ", " << t_raw_packet->f_word_1 << ", " << t_raw_packet->f_word_2 << ", " << t_raw_packet->f_word_3 );
    #endif

                        if( t_roach_packet->f_freq_not_time )
                        {
                            // packet is frequency data

                            t_freq_batch_pkt = t_roach_packet->f_pkt_in_batch;
                            //id_match_sanity_check( t_time_batch_pkt, t_freq_batch_pkt, t_time_session_pkt_counter, t_freq_session_pkt_counter );

                            t_freq_data = out_stream< 1 >().data();
                            t_freq_data->set_pkt_in_session( t_freq_session_pkt_counter++ );
                            ::memcpy( &t_freq_data->packet(), t_roach_packet, f_udp_buffer_size );

                            LDEBUG( plog, "Frequency data received (" << t_size_received << " bytes):  chan = " << t_freq_data->get_digital_id() <<
                                   "  time = " << t_freq_data->get_unix_time() <<
                                   "  id = " << t_freq_data->get_pkt_in_session() <<
                                   "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                                   "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );
                            LDEBUG( plog, "Frequency data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );

                            out_stream< 1 >().set( stream::s_run );
                        }
                        else
                        {
                            // packet is time data

                            t_time_batch_pkt = t_roach_packet->f_pkt_in_batch;
                            //id_match_sanity_check( t_time_batch_pkt, t_freq_batch_pkt, t_time_session_pkt_counter, t_freq_session_pkt_counter );

                            t_time_data = out_stream< 0 >().data();
                            t_time_data->set_pkt_in_session( t_time_session_pkt_counter++ );
                            ::memcpy( &t_time_data->packet(), t_roach_packet, f_udp_buffer_size );

                            LDEBUG( plog, "Time data received (" << t_size_received << " bytes):  chan = " << t_time_data->get_digital_id() <<
                                   "  time = " << t_time_data->get_unix_time() <<
                                   "  id = " << t_time_data->get_pkt_in_session() <<
                                   "  freqNotTime = " << t_time_data->get_freq_not_time() <<
                                   "  bin 0 [0] = " << (unsigned)t_time_data->get_array()[ 0 ][ 0 ] );
                            LDEBUG( plog, "Time data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );

                            out_stream< 0 >().set( stream::s_run );
                        }

                        continue;
                    }
                    else
                    {
                        LDEBUG( plog, "No message received & no error present" );
                        continue;
                    }
                }
            }

            LINFO( plog, "TF ROACH receiver is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output streams" );
            out_stream< 0 >().set( stream::s_stop );
            out_stream< 1 >().set( stream::s_stop );

            LDEBUG( plog, "Exiting output streams" );
            out_stream< 0 >().set( stream::s_exit );
            out_stream< 1 >().set( stream::s_exit );

            return;
        }
        catch( midge::error& e )
        {
            LERROR( plog, "Midge exception caught: " << e.what() );

            LDEBUG( plog, "Stopping output streams" );
            out_stream< 0 >().set( stream::s_stop );
            out_stream< 1 >().set( stream::s_stop );

            LDEBUG( plog, "Exiting output streams" );
            out_stream< 0 >().set( stream::s_exit );
            out_stream< 1 >().set( stream::s_exit );

            return;
        }
        catch( error& e )
        {
            LERROR( plog, "Psyllid exception caught: " << e.what() );

            LDEBUG( plog, "Stopping output streams" );
            out_stream< 0 >().set( stream::s_stop );
            out_stream< 1 >().set( stream::s_stop );

            LDEBUG( plog, "Exiting output streams" );
            out_stream< 0 >().set( stream::s_exit );
            out_stream< 1 >().set( stream::s_exit );

            return;
        }

        // control should not reach here
        LERROR( plog, "Control should not reach this point" );
        return;
    }

    void tf_roach_receiver::finalize()
    {
        out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        return;
    }

    void tf_roach_receiver::check_instruction()
    {
        if( f_paused )
        {
            if( have_instruction() && use_instruction() == midge::instruction::resume )
            {
                LDEBUG( plog, "TF ROACH receiver resuming" );
                out_stream< 0 >().data()->set_unix_time( t_last_packet_time );
                out_stream< 0 >().data()->set_pkt_in_session( 0 );
                out_stream< 1 >().data()->set_pkt_in_session( 0 );
                out_stream< 0 >().set( stream::s_start );
                out_stream< 1 >().set( stream::s_start );
                t_time_session_pkt_counter = 0;
                t_freq_session_pkt_counter = 0;
                f_paused = false;
            }
        }
        else
        {
            if( have_instruction() && use_instruction() == midge::instruction::pause )
            {
                LDEBUG( plog, "TF ROACH receiver pausing" );
                out_stream< 0 >().set( stream::s_stop );
                out_stream< 1 >().set( stream::s_stop );
                f_paused = true;
            }
        }

        return;
    }

    void tf_roach_receiver::do_cancellation()
    {
        LDEBUG( plog, "Canceling tf_roach_receiver" );
        f_server->cancel();
        return;
    }

    void tf_roach_receiver::id_match_sanity_check( uint64_t a_time_batch_pkt, uint64_t a_freq_batch_pkt, uint64_t a_time_session_pkt, uint64_t a_freq_session_pkt )
    {
        if( a_time_batch_pkt == a_freq_batch_pkt )
        {
            if( a_time_session_pkt != a_freq_session_pkt )
            {
                LERROR( plog, "Packet ID mismatch:\n" <<
                        "\tTime batch packet: " << a_time_batch_pkt << '\n' <<
                        "\tFreq batch packet: " << a_freq_batch_pkt << '\n' <<
                        "\tTime session packet: " << a_time_session_pkt << '\n' <<
                        "\tFreq session packet: " << a_freq_session_pkt );
            }
        }
    }



    tf_roach_receiver_builder::tf_roach_receiver_builder() :
            _node_builder< tf_roach_receiver >()
    {
    }

    tf_roach_receiver_builder::~tf_roach_receiver_builder()
    {
    }

    void tf_roach_receiver_builder::apply_config( tf_roach_receiver* a_node, const scarab::param_node& a_config )
    {
        LDEBUG( plog, "Configuring tf_roach_receiver with:\n" << a_config );
        a_node->set_time_length( a_config.get_value( "time-length", a_node->get_time_length() ) );
        a_node->set_freq_length( a_config.get_value( "freq-length", a_node->get_freq_length() ) );
        a_node->set_udp_buffer_size( a_config.get_value( "udp-buffer-size", a_node->get_udp_buffer_size() ) );
        a_node->set_time_sync_tol( a_config.get_value( "time-sync-tol", a_node->get_time_sync_tol() ) );
        const scarab::param_node* t_server_config = a_config.node_at( "server" );
        if( t_server_config == nullptr )
        {
            LDEBUG( plog, "No server config" );
            a_node->set_server_config( nullptr );
        }
        else
        {
            LDEBUG( plog, "Saving server config:\n" << *t_server_config );
            a_node->set_server_config( new scarab::param_node( *t_server_config ) );
        }
        return;
    }


} /* namespace psyllid */
