/*
 * test_server.cc
 *
 *  Created on: Dec 22, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: string_msg_client.go
 */


#include "udp_server_socket.hh"
#include "psyllid_error.hh"
#include "logger.hh"

using namespace psyllid;

LOGGER( plog, "test_udp_server" );

int main()
{
    try
    {
        udp_server_socket t_server( 23530 );

        LINFO( plog, "Server is listening" );

        const size_t t_buff_size = 1024;
        char* t_data = new char[ t_buff_size ];

        ssize_t t_size_received = 0;
        while( t_size_received >= 0 )
        {
            LINFO( plog, "Waiting for a message" );
            t_size_received = t_server.recv( t_data, t_buff_size, 0 );
            if( t_size_received > 0 )
            {
                LINFO( plog, "Message received: " << t_data );
            }
            else
            {
                LDEBUG( plog, "No message received & no error present" );
            }
        }

    }
    catch( std::exception& e )
    {
        LERROR( plog, "Exception caught: " << e.what() );
    }

}
