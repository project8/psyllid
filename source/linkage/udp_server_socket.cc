#include "udp_server_socket.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

LOGGER( plog, "udp_server_socket" );

namespace psyllid
{
    udp_server_socket::udp_server_socket( scarab::param_node* a_config ) :
            udp_server(),
            f_socket( 0 ),
            f_address( nullptr ),
            f_packet_wrapper(),
            f_message_size( 0 ),
            f_pw_mutex()
    {
        int t_port = 23530;
        unsigned t_timeout_sec = 1;
        if( a_config != nullptr )
        {
            LDEBUG( plog, "Received configuration:\n" << *a_config );
            t_port = a_config->get_value( "port", t_port );
            t_timeout_sec = a_config->get_value( "timeout-sec", t_timeout_sec );
        }

        LDEBUG( plog, "Opening udp_server socket on port <" << t_port << ">" );

        //initialize address
        socklen_t t_socket_length = sizeof(sockaddr_in);
        f_address = new sockaddr_in();
        ::memset( f_address, 0, t_socket_length );

        //prepare address
        f_address->sin_family = AF_INET;
        f_address->sin_addr.s_addr = htonl( INADDR_ANY );
        f_address->sin_port = htons( t_port );

        //MTLINFO( pmsg, "address prepared..." );

        //open socket
        f_socket = ::socket( AF_INET, SOCK_DGRAM, 0 );
        if( f_socket < 0 )
        {
            throw error() << "[udp_server] could not create socket:\n\t" << strerror( errno );
            return;
        }

        /* setsockopt: Handy debugging trick that lets
           * us rerun the udp_server immediately after we kill it;
           * otherwise we have to wait about 20 secs.
           * Eliminates "ERROR on binding: Address already in use" error.
           */
        int optval = 1;
        ::setsockopt( f_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

        // Receive timeout
        if( t_timeout_sec > 0 )
        {
            struct timeval t_timeout;
            t_timeout.tv_sec = t_timeout_sec;
            t_timeout.tv_usec = 0;  // Not init'ing this can cause strange errors
            ::setsockopt( f_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_timeout, sizeof(struct timeval) );
        }

        //msg_normal( pmsg, "socket open..." );

        //bind socket
        if( ::bind( f_socket, (const sockaddr*) (f_address), t_socket_length ) < 0 )
        {
            throw error() << "[udp_server] could not bind socket:\n\t" << strerror( errno );
            return;
        }

        LINFO( plog, "Ready to receive messages on port " << t_port );

        return;
    }

    udp_server_socket::~udp_server_socket()
    {
        // delete the message buffer
        delete [] f_packet_wrapper.f_message;

        //clean up udp_server address
        delete f_address;

        //close udp_server socket
        ::close( f_socket );
    }

    void udp_server_socket::activate()
    {
        return;
    }

    void udp_server_socket::execute()
    {
        while( ! f_canceled.load() )
        {
            f_pw_mutex.lock();
            f_packet_wrapper.f_ready = false;
            ssize_t t_recv_size = 0;
            try
            {
                t_recv_size = recv( f_packet_wrapper.f_message, f_message_size, 0 );
                if( t_recv_size > 0 )
                {
                    f_packet_wrapper.f_size = t_recv_size;
                    f_packet_wrapper.f_ready = true;
                }
            }
            catch( std::exception& e )
            {
                if( t_recv_size < 0 ) LERROR( plog, e.what() );
            }
            f_pw_mutex.unlock();
        }

        return;
    }

    ssize_t udp_server_socket::get_next_packet( char* a_message, size_t a_size )
    {
        ssize_t t_bytes_copied = 0;
        f_pw_mutex.lock();
        if( f_packet_wrapper.f_ready )
        {
            t_bytes_copied = std::min( a_size, f_packet_wrapper.f_size );
            ::memcpy( a_message, f_packet_wrapper.f_message, t_bytes_copied );
        }
        f_pw_mutex.unlock();
        return t_bytes_copied;
    }

    ssize_t udp_server_socket::recv( char* a_message, size_t a_size, int flags, int& ret_errno )
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
            throw error() << "No message present";
        }
        else  // t_recv_size < 0 && f_last_errno != EWOULDBLOCK && f_last_errno != EAGAIN
        {
            throw error() << "Unable to receive; error message: " << strerror( f_last_errno ) << "\n";
        }
    }

    void udp_server_socket::deactivate()
    {
        return;
    }
}
