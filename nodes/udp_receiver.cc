/*
 * udp_receiver.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "udp_receiver.hh"
#include "server.hh"

namespace psyllid
{

    udp_receiver::udp_receiver() :
            f_length( 10 ),
            f_port( 23530 )
    {
    }

    udp_receiver::~udp_receiver()
    {
    }

    void udp_receiver::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void udp_receiver::execute()
    {
        roach_packet* t_current_data;

        t_current_data = out_stream< 0 >().data();

        // do any preparation of the data

        try
        {
            server t_server( f_port );

            pmsg( s_debug ) << "Server is listening" <<eom;

            const size_t t_buff_size = 1024;
            char* t_data = new char[ t_buff_size ];

            out_stream< 0 >().set( stream::s_start );

            ssize_t t_size_received = 0;
            while( t_size_received >= 0 )
            {
                pmsg( s_normal ) << "Waiting for ROACH packets" << eom;

                if( (out_stream< 0 >().get() == stream::s_stop) || (false /* some other condition for stopping */) )
                {
                    out_stream< 0 >().set( stream::s_stop );
                    out_stream< 0 >().set( stream::s_exit );
                    return;
                }

                t_size_received = t_server.recv( t_data, t_buff_size, 0 );

                if( t_size_received > 0 )
                {
                    pmsg( s_normal ) << "Message received: " << t_data << eom;

                    t_current_data = out_stream< 0 >().data();

                    // load the data object

                    out_stream< 0 >().set( stream::s_run );

                }
                else if( t_size_received == 0 )
                {
                    pmsg( s_debug ) << "No message received & no error present" << eom;
                }
                else
                {
                    pmsg( s_error ) << "An error occurred while receiving a packet" << eom;
                    out_stream< 0 >().set( stream::s_stop );
                    out_stream< 0 >().set( stream::s_exit );
                    return;
                }
            }
        }
        catch( error& e )
        {
            pmsg( s_error ) << "Exception caught: " << e.what() << eom;
        }

        return;
    }

    void udp_receiver::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }

} /* namespace psyllid */
