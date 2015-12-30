/*
 * udp_receiver.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "udp_receiver.hh"

#include "psyllidmsg.hh"
#include "server.hh"

#include <memory>


namespace psyllid
{

    udp_receiver::udp_receiver() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            f_port( 23530 ),
            f_udp_buffer_size( 1024 )
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

    void udp_receiver::execute()
    {
        time_data* t_time_data = nullptr;
        freq_data* t_freq_data = nullptr;

        count_t t_id = 0;

        try
        {
            server t_server( f_port );

            pmsg( s_debug ) << "Server is listening" << eom;

            std::unique_ptr< char[] > t_data( new char[ f_udp_buffer_size ] );

            //t_time_data = out_stream< 0 >().data();
            //t_time_data->array()->resize( 1 );

            //t_freq_data = out_stream< 1 >().data();
            //t_freq_data->array()->resize( 1 );

            out_stream< 0 >().set( stream::s_start );
            out_stream< 1 >().set( stream::s_start );

            ssize_t t_size_received = 0;
            while( t_size_received >= 0 )
            {
                if( (out_stream< 0 >().get() == stream::s_stop) ||
                        (out_stream< 1 >().get() == stream::s_stop) ||
                        (false /* some other condition for stopping */) )
                {
                    pmsg( s_normal ) << "UDP Receiver is stopping" << eom;
                    out_stream< 0 >().set( stream::s_stop );
                    out_stream< 1 >().set( stream::s_stop );
                    out_stream< 0 >().set( stream::s_exit );
                    out_stream< 1 >().set( stream::s_exit );
                    return;
                }

                pmsg( s_normal ) << "Waiting for ROACH packets" << eom;

                t_size_received = t_server.recv( t_data.get(), f_udp_buffer_size, 0 );

                if( t_size_received > 0 )
                {
                    t_time_data = out_stream< 0 >().data();
                    t_time_data->array()->resize( 1 );

                    t_freq_data = out_stream< 1 >().data();
                    t_freq_data->array()->resize( 1 );

                    memcpy( t_time_data->array()->data(), t_data.get(), sizeof( int8_t ) );
                    memcpy( t_freq_data->array()->data(), t_data.get() + sizeof( int8_t ), sizeof( real_t ) );

                    t_time_data->set_id( t_id );
                    t_freq_data->set_id( t_id++ );

                    pmsg( s_debug ) << "Data received (" << t_size_received << " bytes): " <<
                            (int)(*t_time_data->array())[0] << "(" << t_time_data->get_id() << ") --> " <<
                            (*t_freq_data->array())[0] << "(" << t_freq_data->get_id() << ")" << eom;

                    out_stream< 0 >().set( stream::s_run );
                    out_stream< 1 >().set( stream::s_run );

                    continue;
                }
                else if( t_size_received == 0 )
                {
                    pmsg( s_debug ) << "No message received & no error present" << eom;
                    continue;
                }
                else
                {
                    pmsg( s_error ) << "An error occurred while receiving a packet" << eom;
                    out_stream< 0 >().set( stream::s_stop );
                    out_stream< 1 >().set( stream::s_stop );
                    out_stream< 0 >().set( stream::s_exit );
                    out_stream< 1 >().set( stream::s_exit );
                    return;
                }
            }
        }
        catch( error& e )
        {
            pmsg( s_error ) << "Exception caught: " << e.what() << eom;
            out_stream< 0 >().set( stream::s_stop );
            out_stream< 1 >().set( stream::s_stop );
            out_stream< 0 >().set( stream::s_exit );
            out_stream< 1 >().set( stream::s_exit );
        }

        return;
    }

    void udp_receiver::finalize()
    {
        out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        return;
    }

} /* namespace psyllid */
