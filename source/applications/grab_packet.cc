/*
 * grab_packet.cc
 *
 *  Created on: Dec 22, 2016
 *      Author: N. Oblath
 *
 *  Grabs and parses a ROACH packet
 *
 *  Usage: > grab_packet [options]
 *
 *  Parameters:
 *    - port: (uint) port number to listen on for packets
 *    - interface: (string) network interface name to listen on for packets; this is only needed if using the FPA receiver; default is "eth1"
 *    - ip: (string) IP address to listen on for packets; this is only needed if using the socket receiver; default is "127.0.0.1"
 *    - fpa: (null) Flag to request use of the FPA receiver; only valid on linux machines
 */


#include "psyllid_error.hh"

#include "freq_data.hh"
#include "time_data.hh"

#include "configurator.hh"
#include "logger.hh"
#include "param.hh"

#include <arpa/inet.h>
#include <errno.h>
#include <memory>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>


using namespace psyllid;

LOGGER( plog, "grab_packet" );


bool ProcessUnknownPacket( uint8_t* a_buffer );
bool ProcessROACHPacket( uint8_t* a_buffer );


int main( int argc, char** argv )
{
    try
    {
        scarab::param_node t_default_config;
        t_default_config.add( "n", new scarab::param_value( 1U ) );
        t_default_config.add( "ip", new scarab::param_value( "127.0.0.1" ) );
        t_default_config.add( "port", new scarab::param_value( 23530U ) );
        t_default_config.add( "interface", new scarab::param_value( "eth1" ) );
        t_default_config.add( "timeout", new scarab::param_value( 10U ) );
        t_default_config.add( "max-packet-size", new scarab::param_value( 16384U ) );
        t_default_config.add( "packet-type", new scarab::param_value( "roach" ) );

        scarab::configurator t_configurator( argc, argv, &t_default_config );

        unsigned t_n_packets( t_configurator.get< unsigned >( "n" ) );
        std::string t_ip( t_configurator.get< std::string >( "ip" ) );
        unsigned t_port = t_configurator.get< unsigned >( "port" );
        std::string t_interface( t_configurator.get< std::string >( "interface" ) );
        unsigned t_timeout_sec = t_configurator.get< unsigned >( "timeout" );
        //bool t_use_fpa( t_configurator.config().has( "fpa" ) );
        unsigned t_max_packet_size = t_configurator.get< unsigned >( "max-packet-size" );

        std::string t_packet_type( t_configurator.get< std::string >( "packet-type" ) );
        bool (*t_proc_pkt_func)( uint8_t* ) = nullptr;
        if( t_packet_type == "roach" )
        {
            t_proc_pkt_func = &ProcessROACHPacket;
            LDEBUG( plog, "Processing packets as ROACH packets" );
        }
        else if( t_packet_type == "unknown" )
        {
            t_proc_pkt_func= &ProcessUnknownPacket;
            LDEBUG( plog, "Processing packets as unknown packets" );
        }
        else
        {
            LERROR( plog, "Unknown packet type supplied: " << t_packet_type );
            return -1;
        }


        LDEBUG( plog, "Opening UDP socket receiving at " << t_ip << ":" << t_port );

        //initialize address
        socklen_t t_socket_length = sizeof(sockaddr_in);
        sockaddr_in* t_address = new sockaddr_in();
        ::memset( t_address, 0, t_socket_length );

        //prepare address
        t_address->sin_family = AF_INET;
        t_address->sin_addr.s_addr = inet_addr( t_ip.c_str() );
        if( t_address->sin_addr.s_addr == INADDR_NONE )
        {
            LERROR( plog, "Invalid IP address\n" );
            return -1;
        }
        t_address->sin_port = htons( t_port );

        //MTLINFO( pmsg, "address prepared..." );

        //open socket
        int t_socket = ::socket( AF_INET, SOCK_DGRAM, 0 );
        if( t_socket < 0 )
        {
            LERROR( plog, "Could not create socket:\n\t" << strerror( errno ) );
            return-1 ;
        }

        /* setsockopt: Handy debugging trick that lets
           * us rerun the udp_server immediately after we kill it;
           * otherwise we have to wait about 20 secs.
           * Eliminates "ERROR on binding: Address already in use" error.
           */
        int optval = 1;
        ::setsockopt( t_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

        // Receive timeout
        if( t_timeout_sec > 0 )
        {
            struct timeval t_timeout;
            t_timeout.tv_sec = t_timeout_sec;
            t_timeout.tv_usec = 0;  // Not init'ing this can cause strange errors
            ::setsockopt( t_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_timeout, sizeof(struct timeval) );
        }

        //msg_normal( pmsg, "socket open..." );

        //bind socket
        if( ::bind( t_socket, (const sockaddr*) (t_address), t_socket_length ) < 0 )
        {
            LERROR( plog, "Could not bind socket:\n\t" << strerror( errno ) );
            return -1;
        }



        uint8_t* t_buffer = new uint8_t [t_max_packet_size];

        for( unsigned i_packet = 0; i_packet < t_n_packets; ++i_packet )
        {
            LINFO( plog, "Waiting for next packet" );
            ssize_t t_size_received = 0;
            while( t_size_received <= 0 )
            {
                t_size_received = ::recv( t_socket, (void*)t_buffer, t_max_packet_size, 0 );

                if( t_size_received > 0 )
                {
                    LINFO( plog, "Packet received (" << t_size_received << " bytes)" );

                    if( ! (*t_proc_pkt_func)( t_buffer ) )
                    {
                        LWARN( plog, "Packet not processed correctly" );
                    }
                }
            }
        }



        //clean up udp_server address
        if( t_address != nullptr )
        {
            delete t_address;
            t_address = nullptr;
        }

        //close udp_server socket
        if( t_socket != 0 )
        {
            ::close( t_socket );
            t_socket = 0;
        }



    }
    catch( std::exception& e )
    {
        LERROR( plog, "Caught an exception: " << e.what() );
        return -1;
    }

    return 0;
}


bool ProcessROACHPacket( uint8_t* a_buffer )
{
    byteswap_inplace( reinterpret_cast< raw_roach_packet* >( a_buffer ) );
    roach_packet* t_roach_packet = reinterpret_cast< roach_packet* >( a_buffer );

    // debug purposes only
#ifndef NDEBUG
    raw_roach_packet* t_raw_packet = reinterpret_cast< raw_roach_packet* >( a_buffer );
    LDEBUG( plog, "Raw packet header: " << std::hex << t_raw_packet->f_word_0 << ", " << t_raw_packet->f_word_1 << ", " << t_raw_packet->f_word_2 << ", " << t_raw_packet->f_word_3 );
#endif

    LINFO( plog, "ROACH data received:\n"
           "  digital_id (channel) = " << t_roach_packet->f_digital_id << '\n' <<
           "  unix time = " << t_roach_packet->f_unix_time << '\n' <<
           "  pkt_batch = " << t_roach_packet->f_pkt_in_batch << '\n' <<
           "  freqNotTime = " << (int)t_roach_packet->f_freq_not_time << '\n' <<
           "  if id = " << t_roach_packet->f_if_id << '\n' <<
           "  first 6 bins = " << (int)t_roach_packet->f_data[ 0 ] << ", " << (int)t_roach_packet->f_data[ 1 ] << ";  " << (int)t_roach_packet->f_data[ 2 ] << ", " << (int)t_roach_packet->f_data[ 3 ] << ";  " << (int)t_roach_packet->f_data[ 4 ] << ", " << (int)t_roach_packet->f_data[ 5 ] );


    return true;
}

bool ProcessUnknownPacket( uint8_t* /*a_buffer*/ )
{
    return true;
}


