/*
 * test_server.cc
 *
 *  Created on: Dec 22, 2015
 *      Author: nsoblath
 */


#include "error.hh"
#include "psyllidmsg.hh"
#include "server.hh"



using namespace midge;
using namespace psyllid;

int main()
{
    try
    {
        server t_server( 23530 );

        psyllidmsg( s_normal ) << "Server is listening" <<eom;

        psyllidmsg( s_normal ) << "Waiting for connections . . ." << eom;

        const size_t t_buff_size = 1024;
        char* t_data = new char[ t_buff_size ];

        ssize_t t_size_received = 0;
        while( t_size_received >= 0 )
        {
            psyllidmsg( s_normal ) << "Waiting for a message" << eom;
            t_size_received = t_server.recv( t_data, t_buff_size, 0 );
            if( t_size_received > 0 )
            {
                psyllidmsg( s_normal ) << "Message received: " << t_data << eom;
            }
            else
            {
                psyllidmsg( s_debug ) << "No message received & no error present" << eom;
            }
        }

    }
    catch( error& e )
    {
        psyllidmsg( s_error ) << "Exception caught: " << e.what() << eom;
    }

}
