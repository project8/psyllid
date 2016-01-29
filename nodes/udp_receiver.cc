/*
 * udp_receiver.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "udp_receiver.hh"

#include "logger.hh"

#include <memory>
#include "udp_server.hh"

using midge::stream;

namespace psyllid
{
    REGISTER_NODE( udp_receiver, "udp-receiver" );

    LOGGER( plog, "udp_receiver" );

    udp_receiver::udp_receiver() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            f_port( 23530 ),
            f_udp_buffer_size( sizeof( roach_packet ) )
    {
    }

    udp_receiver::~udp_receiver()
    {
    }

    void udp_receiver::initialize()
    {
        out_buffer< 0 >().initialize( f_time_length );
        out_buffer< 1 >().initialize( f_freq_length );
        return;
    }

    void udp_receiver::execute( midge::shared_cancel_t a_canceled )
    {
        time_data* t_time_data = nullptr;
        freq_data* t_freq_data = nullptr;

        try
        {
            udp_server t_server( f_port );

            DEBUG( plog, "Server is listening" );

            std::unique_ptr< char[] > t_buffer_ptr( new char[ f_udp_buffer_size ] );

            out_stream< 0 >().set( stream::s_start );
            out_stream< 1 >().set( stream::s_start );

            ssize_t t_size_received = 0;
            while( t_size_received >= 0 && ! a_canceled->load() )
            {
                if( (out_stream< 0 >().get() == stream::s_stop) ||
                        (out_stream< 1 >().get() == stream::s_stop) ||
                        (false /* some other condition for stopping */) )
                {
                    INFO( plog, "UDP Receiver is stopping" );
                    break;
                }

                INFO( plog, "Waiting for ROACH packets" );

                t_size_received = t_server.recv( t_buffer_ptr.get(), f_udp_buffer_size, 0 );

                if( t_size_received > 0 )
                {
                    byteswap_inplace( reinterpret_cast< raw_roach_packet* >( t_buffer_ptr.get() ) );
                    roach_packet* t_roach_packet = reinterpret_cast< roach_packet* >( t_buffer_ptr.get() );

                    if( t_roach_packet->f_freq_not_time )
                    {
                        // packet is frequency data
                        t_freq_data = out_stream< 1 >().data();
                        memcpy( &t_freq_data->packet(), t_roach_packet, f_udp_buffer_size );

                        DEBUG( plog, "Frequency data received (" << t_size_received << " bytes):  chan = " << t_freq_data->get_digital_id() <<
                               "  id = " << t_freq_data->get_pkt_in_batch() );

                        out_stream< 1 >().set( stream::s_run );
                    }
                    else
                    {
                        // packet is time data
                        t_time_data = out_stream< 0 >().data();
                        memcpy( &t_time_data->packet(), t_roach_packet, f_udp_buffer_size );

                        DEBUG( plog, "Time data received (" << t_size_received << " bytes):  chan = " << t_time_data->get_digital_id() <<
                               "  id = " << t_time_data->get_pkt_in_batch() );

                        out_stream< 0 >().set( stream::s_run );
                    }

                    continue;
                }
                else if( t_size_received == 0 )
                {
                    DEBUG( plog, "No message received & no error present" );
                    continue;
                }
                else
                {
                    ERROR( plog, "An error occurred while receiving a packet" );
                    break;
                }
            }
            // normal exit condition
            out_stream< 0 >().set( stream::s_stop );
            out_stream< 1 >().set( stream::s_stop );
            out_stream< 0 >().set( stream::s_exit );
            out_stream< 1 >().set( stream::s_exit );
            return;
        }
        catch( midge::error& e )
        {
            ERROR( plog, "Exception caught: " << e.what() );
            out_stream< 0 >().set( stream::s_stop );
            out_stream< 1 >().set( stream::s_stop );
            out_stream< 0 >().set( stream::s_exit );
            out_stream< 1 >().set( stream::s_exit );
        }

        // control should not reach here
        ERROR( plog, "Control should not reach this point" );
        return;
    }

    void udp_receiver::finalize()
    {
        out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        return;
    }

} /* namespace psyllid */
