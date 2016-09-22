/*
 * packet_distributor.cc
 *
 *  Created on: Aug 23, 2016
 *      Author: nsoblath
 */

#include "packet_distributor.hh"

#include "psyllid_error.hh"

#include "logger.hh"

#include <arpa/inet.h>
#include <linux/udp.h>

namespace psyllid
{
    LOGGER( plog, "packet_distributor" );


    packet_distributor::packet_distributor() :
            f_ip_buffer( nullptr ),
            f_ip_pkt_iterator(),
            f_udp_buffers()
    {
    }

    packet_distributor::~packet_distributor()
    {
        f_ip_pkt_iterator.release();

        while( ! f_udp_buffers.empty() )
        {
            f_udp_buffers.begin()->second.f_iterator.release();
            f_udp_buffers.erase( f_udp_buffers.begin() );
        }
    }

    bool packet_distributor::open_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size )
    {
        buffer_map::iterator t_it = f_udp_buffers.find( a_port );
        if( f_udp_buffers.find( a_port ) != f_udp_buffers.end() )
        {
            LWARN( plog, "Port <" << a_port << "> is already open" );
            return false;
        }

        // Initialize the udp_buffer struct in the map
        f_udp_buffers[ a_port ] = udp_buffer{ packet_buffer( a_buffer_size ), pb_iterator() };

        // attach the write iterator
        f_udp_buffers[ a_port ].f_iterator.attach( &f_udp_buffers[ a_port ].f_buffer );

        // attach the read iterator
        a_iterator.attach( &f_udp_buffers[ a_port ].f_buffer );

        return true;
    }

    bool packet_distributor::close_port( unsigned a_port )
    {
        buffer_map::iterator t_it = f_udp_buffers.find( a_port );
        if( t_it == f_udp_buffers.end() )
        {
            LWARN( plog, "Port <" << a_port << "> is not open" );
            return true;
        }

        f_udp_buffers.erase( t_it );
        return true;
    }

    void packet_distributor::initialize()
    {
        if( ! f_ip_pkt_iterator.attach( f_ip_buffer ) )
        {
            throw error() << "[packet_distributor] Unable to attach iterator to buffer";
        }
        return;
    }

    void packet_distributor::execute()
    {

        // move the read iterator up until it runs up against a blocked packet
        while( +f_ip_pkt_iterator );
        f_ip_pkt_iterator.set_reading();

        // set write packet statuses
        for( buffer_map::iterator t_it = f_udp_buffers.begin(); t_it != f_udp_buffers.end(); ++t_it )
        {
            t_it->second.f_iterator.set_writing();
        }

        while( ! is_canceled() )
        {
            // advance the iterator; blocks until next packet is available
            ++f_ip_pkt_iterator;

            // check for cancelation
            if( is_canceled() )
            {
                break;
            }

            // if the IP packet we're on is unused, skip it
            if( f_ip_pkt_iterator.is_unused() )
            {
                continue;
            }

            // read the IP packet
            f_ip_pkt_iterator.set_reading();

            distribute_packet();

            f_ip_pkt_iterator.set_unused();
        }

        return;
    }

    bool packet_distributor::distribute_packet()
    {
        // this doesn't appear to be defined anywhere in standard linux headers, at least as far as I could find
        static const unsigned t_udp_hdr_len = 8;

        udphdr* t_udp_hdr = reinterpret_cast< udphdr* >( f_ip_pkt_iterator->ptr() );

        // get port number
        unsigned t_port = htons(t_udp_hdr->dest);

        // check for port number in the buffer map
        buffer_map::iterator t_it = f_udp_buffers.find( t_port );
        if( t_it == f_udp_buffers.end() )
        {
            LDEBUG( plog, "Packet addressed to unopened port: " << t_port );
            return false;
        }

        t_it->second.f_iterator.set_writing();

        // copy the UPD packet from the IP packet into the appropriate buffer
        t_it->second.f_iterator->memcpy( reinterpret_cast< uint8_t* >( t_udp_hdr + t_udp_hdr_len ), htons(t_udp_hdr->len) - t_udp_hdr_len );

        t_it->second.f_iterator.set_written();

        return true;
    }

} /* namespace psyllid */
