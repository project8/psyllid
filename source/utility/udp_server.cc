#include "midge_error.hh"
#include "udp_server.hh"

#include "logger.hh"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

LOGGER( plog, "udp_server" );

namespace psyllid
{
    int udp_server::f_last_errno = 0;

    udp_server::udp_server( const int& a_port, unsigned a_timeout_sec ) :
            f_socket( 0 ),
            f_address( nullptr )
    {
        LDEBUG( plog, "Opening udp_server socket on port <" << a_port << ">" );

        //initialize address
        socklen_t t_socket_length = sizeof(sockaddr_in);
        f_address = new sockaddr_in();
        ::memset( f_address, 0, t_socket_length );

        //prepare address
        f_address->sin_family = AF_INET;
        f_address->sin_addr.s_addr = htonl( INADDR_ANY );
        f_address->sin_port = htons( a_port );

        //MTLINFO( pmsg, "address prepared..." );

        //open socket
        f_socket = ::socket( AF_INET, SOCK_DGRAM, 0 );
        if( f_socket < 0 )
        {
            throw ::midge::error() << "[udp_server] could not create socket:\n\t" << strerror( errno );
            return;
        }

        /* setsockopt: Handy debugging trick that lets
           * us rerun the udp_server immediately after we kill it;
           * otherwise we have to wait about 20 secs.
           * Eliminates "ERROR on binding: Address already in use" error.
           */
        //int optval = 1;
        //::setsockopt( f_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

        // Receive timeout
        if( a_timeout_sec > 0 )
        {
            struct timeval t_timeout;
            t_timeout.tv_sec = a_timeout_sec;
            t_timeout.tv_usec = 0;  // Not init'ing this can cause strange errors
            ::setsockopt( f_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_timeout, sizeof(struct timeval) );
        }

        //msg_normal( pmsg, "socket open..." );

        //bind socket
        if( ::bind( f_socket, (const sockaddr*) (f_address), t_socket_length ) < 0 )
        {
            throw midge::error() << "[udp_server] could not bind socket:\n\t" << strerror( errno );
            return;
        }

        LINFO( plog, "Ready to receive messages on port " << a_port );

        return;
    }

    udp_server::~udp_server()
    {
        //clean up udp_server address
        delete f_address;

        //close udp_server socket
        ::close( f_socket );
    }

    ssize_t udp_server::recv( char* a_message, size_t a_size, int flags, int& ret_errno )
    {
        ssize_t t_recv_size = ::recv( f_socket, (void*)a_message, a_size, flags );
        if( t_recv_size > 0 )
        {
            return t_recv_size;
        }
        f_last_errno = errno;
        ret_errno = errno;
        if( f_last_errno == EWOULDBLOCK || f_last_errno == EAGAIN )
        {
            // recv timed out without anything being available to receive
            // nothing seems to be wrong with the socket
            return t_recv_size;
        }
        else if( t_recv_size == 0 )
        {
            throw midge::error() << "No message present";
        }
        else  // t_recv_size < 0 && f_last_errno != EWOULDBLOCK && f_last_errno != EAGAIN
        {
            throw midge::error() << "Unable to receive; error message: " << strerror( f_last_errno ) << "\n";
        }
    }

}
