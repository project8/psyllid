#include "server.hh"

#include "midge_error.hh"

#include "logger.hh"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

LOGGER( plog, "server" );

namespace psyllid
{
    int server::f_last_errno = 0;

    server::server( const int& a_port ) :
            f_socket( 0 ),
            f_address( nullptr )
    {
        //msg_normal( pmsg, "opening server socket on port <" << a_port << ">" );

        //initialize address
        socklen_t t_socket_length = sizeof(sockaddr_in);
        f_address = new sockaddr_in();
        ::memset( f_address, 0, t_socket_length );

        //prepare address
        f_address->sin_family = AF_INET;
        f_address->sin_addr.s_addr = htonl( INADDR_ANY );
        f_address->sin_port = htons( a_port );

        //MTINFO( pmsg, "address prepared..." );

        //open socket
        f_socket = ::socket( AF_INET, SOCK_DGRAM, 0 );
        if( f_socket < 0 )
        {
            throw ::midge::error() << "[server] could not create socket:\n\t" << strerror( errno );
            return;
        }

        /* setsockopt: Handy debugging trick that lets
           * us rerun the server immediately after we kill it;
           * otherwise we have to wait about 20 secs.
           * Eliminates "ERROR on binding: Address already in use" error.
           */
        //int optval = 1;
        //::setsockopt( f_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));


        //msg_normal( pmsg, "socket open..." );

        //bind socket
        if( ::bind( f_socket, (const sockaddr*) (f_address), t_socket_length ) < 0 )
        {
            throw midge::error() << "[server] could not bind socket:\n\t" << strerror( errno );
            return;
        }

        INFO( plog, "Ready to receive messages" );

        return;
    }

    server::~server()
    {
        //clean up server address
        delete f_address;

        //close server socket
        ::close( f_socket );
    }

    ssize_t server::recv( char* a_message, size_t a_size, int flags, int& ret_errno )
    {
        ssize_t t_recv_size = ::recv( f_socket, (void*)a_message, a_size, flags );
        if( t_recv_size > 0 )
        {
            return t_recv_size;
        }
        f_last_errno = errno;
        ret_errno = errno;
        if( t_recv_size == 0 && f_last_errno != EWOULDBLOCK && f_last_errno != EAGAIN )
        {
            throw midge::error() << "No message present";
        }
        else if( t_recv_size < 0 )
        {
            throw midge::error() << "Unable to receive; error message: " << strerror( f_last_errno ) << "\n";
        }
        // at this point t_recv_size must be 0, and errno is either EWOULDBLOCK or EAGAIN,
        // which means that there was no data available to receive, but nothing seems to be wrong with the connection
        return t_recv_size;
    }

}
