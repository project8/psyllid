/*
 * packet_receiver_socket.cc
 *
 *  Created on: Nov 2, 2016
 *      Author: nsoblath
 */

#include "packet_receiver_socket.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <arpa/inet.h>
#include <errno.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( packet_receiver_socket, "packet-receiver-socket", packet_receiver_socket_binding );

    LOGGER( plog, "packet_receiver_socket" );

    packet_receiver_socket::packet_receiver_socket() :
            f_length( 10 ),
            f_max_packet_size( 1048576 ),
            f_port( 23530 ),
            f_ip( "127.0.0.1" ),
            f_timeout_sec( 1 ),
            f_socket( 0 ),
            f_address( nullptr ),
            f_last_errno( 0 )
    {
    }

    packet_receiver_socket::~packet_receiver_socket()
    {
        cleanup_socket();
    }

    void packet_receiver_socket::initialize()
    {
        out_buffer< 0 >().initialize( f_length );

        LDEBUG( plog, "Opening UDP socket receiving at " << f_ip << ":" << f_port );

        //initialize address
        socklen_t t_socket_length = sizeof(sockaddr_in);
        f_address = new sockaddr_in();
        ::memset( f_address, 0, t_socket_length );

        //prepare address
        f_address->sin_family = AF_INET;
        f_address->sin_addr.s_addr = inet_addr( f_ip.c_str() );
        if( f_address->sin_addr.s_addr == INADDR_NONE )
        {
            throw error() << "[packet_receiver_socket] invalid IP address\n";
            return;
        }
        f_address->sin_port = htons( f_port );

        //MTLINFO( pmsg, "address prepared..." );

        //open socket
        f_socket = ::socket( AF_INET, SOCK_DGRAM, 0 );
        if( f_socket < 0 )
        {
            throw error() << "[packet_receiver_socket] could not create socket:\n\t" << strerror( errno );
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
        if( f_timeout_sec > 0 )
        {
            struct timeval t_timeout;
            t_timeout.tv_sec = f_timeout_sec;
            t_timeout.tv_usec = 0;  // Not init'ing this can cause strange errors
            ::setsockopt( f_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_timeout, sizeof(struct timeval) );
        }

        //msg_normal( pmsg, "socket open..." );

        //bind socket
        if( ::bind( f_socket, (const sockaddr*) (f_address), t_socket_length ) < 0 )
        {
            throw error() << "[packet_receiver_socket] could not bind socket:\n\t" << strerror( errno );
            return;
        }

        LINFO( plog, "Ready to receive messages at port " << f_ip << ":" << f_port );

        return;
    }

    void packet_receiver_socket::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the packet_receiver_socket" );

            memory_block* t_block = nullptr;

            //LDEBUG( plog, "Server is listening" );

            if( ! out_stream< 0 >().set( stream::s_start ) ) return;

            ssize_t t_size_received = 0;

            LINFO( plog, "Starting main loop; waiting for packets" );
            while( ! is_canceled() )
            {
                t_block = out_stream< 0 >().data();
                t_block->resize( f_max_packet_size );

                t_size_received = 0;

                if( (out_stream< 0 >().get() == stream::s_stop) )
                {
                    LWARN( plog, "Output stream(s) have stop condition" );
                    break;
                }

                LTRACE( plog, "Waiting for packets" );

                // inner loop over packet-receive timeouts
                while( t_size_received <= 0 && ! is_canceled() )
                {
                    t_size_received = ::recv( f_socket, (void*)t_block->block(), f_max_packet_size, 0 );

                    if( t_size_received > 0 )
                    {
                        LTRACE( plog, "Packet received (" << t_size_received << " bytes)" );
                        LTRACE( plog, "Packet written to stream index <" << out_stream< 0 >().get_current_index() << ">" );

                        t_block->set_n_bytes_used( t_size_received );

                        if( ! out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            break;
                        }
                        break;
                    }

                    f_last_errno = errno;
                    if( f_last_errno == EWOULDBLOCK || f_last_errno == EAGAIN )
                    {
                        // recv timed out without anything being available to receive
                        // nothing seems to be wrong with the socket
                        break;
                    }
                    else if( t_size_received == 0 )
                    {
                        LWARN( plog, "No message present" );
                    }
                    else  // t_size_received < 0 && f_last_errno != EWOULDBLOCK && f_last_errno != EAGAIN
                    {
                        LWARN( "Unable to receive; error message: " << strerror( f_last_errno ) );
                    }
                }
            }

            LINFO( plog, "Packet receiver is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output streams" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( plog, "Exiting output streams" );
            out_stream< 0 >().set( stream::s_exit );

            return;
        }
        catch(...)
        {
            a_midge->throw_ex( std::current_exception() );
        }
    }

    void packet_receiver_socket::finalize()
    {
        out_buffer< 0 >().finalize();

        cleanup_socket();

        return;
    }

    void packet_receiver_socket::cleanup_socket()
    {
        //clean up udp_server address
        if( f_address != nullptr )
        {
            delete f_address;
            f_address = nullptr;
        }

        //close udp_server socket
        if( f_socket != 0 )
        {
            ::close( f_socket );
            f_socket = 0;
        }

        return;
    }


    packet_receiver_socket_binding::packet_receiver_socket_binding() :
            _node_binding< packet_receiver_socket, packet_receiver_socket_binding >()
    {
    }

    packet_receiver_socket_binding::~packet_receiver_socket_binding()
    {
    }

    void packet_receiver_socket_binding::do_apply_config( packet_receiver_socket* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring packet_receiver_socket with:\n" << a_config );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        a_node->set_port( a_config.get_value( "port", a_node->get_port() ) );
        a_node->ip() = a_config.get_value( "ip", a_node->ip() );
        a_node->set_timeout_sec( a_config.get_value( "timeout-sec", a_node->get_timeout_sec() ) );
        return;
    }

    void packet_receiver_socket_binding::do_dump_config( const packet_receiver_socket* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for packet_receiver_socket" );
        a_config.add( "length", scarab::param_value( a_node->get_length() ) );
        a_config.add( "port", scarab::param_value( a_node->get_port() ) );
        a_config.add( "ip", scarab::param_value( a_node->ip() ) );
        a_config.add( "timeout-sec", scarab::param_value( a_node->get_timeout_sec() ) );
        return;
    }


} /* namespace psyllid */
