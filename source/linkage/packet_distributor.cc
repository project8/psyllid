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
#include <sstream>

namespace psyllid
{
    LOGGER( plog, "packet_distributor" );

    packet_distributor::udp_buffer::udp_buffer( unsigned a_size, unsigned a_packet_size, const std::string& a_name ) :
            f_buffer( a_size, a_packet_size ),
            f_iterator( a_name )
    {}

    packet_distributor::packet_distributor( const std::string& a_interface ) :
            f_ip_buffer( nullptr ),
            f_ip_pkt_iterator( a_interface + std::string("ip_reader") ),
            f_udp_buffers()
    {
    }

    packet_distributor::~packet_distributor()
    {
        f_ip_pkt_iterator.release();

        while( ! f_udp_buffers.empty() )
        {
            f_udp_buffers.begin()->second->f_iterator.release();
            delete f_udp_buffers.begin()->second;
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

        LDEBUG( plog, "Opening port <" << a_port << ">" );

        // Initialize the udp_buffer struct in the map
        //f_udp_buffers[ a_port ] = udp_buffer{ packet_buffer( a_buffer_size ), pb_iterator() };
        std::stringstream a_conv;
        a_conv << a_port;
	std::string t_port_str( "p" + a_conv.str() );
        udp_buffer* t_new_buffer = new udp_buffer( a_buffer_size, 0, t_port_str + std::string("_udp_writer") );
        f_udp_buffers[ a_port ] = t_new_buffer;
        //f_udp_buffers.emplace( std::make_pair( a_port, udp_buffer( a_buffer_size ) ) );
        LWARN( plog, "Initialized outbound buffer for port <" << a_port << ">; packet 0 ptr is " << f_udp_buffers.at( a_port )->f_buffer.get_packet(0) );

        // attach the write iterator
        f_udp_buffers.at( a_port )->f_iterator.attach( &f_udp_buffers.at( a_port )->f_buffer );
        LWARN( plog, "Attached writer iterator; current packet (at " << f_udp_buffers.at( a_port )->f_iterator.index()
               << ") ptr is " << f_udp_buffers.at( a_port )->f_iterator.object() );

        // attach the read iterator
        a_iterator.name() = t_port_str + std::string("_udp_reader");
        a_iterator.attach( &f_udp_buffers.at( a_port )->f_buffer );

        LDEBUG( plog, "Port <" << a_port << "> is open; distributor now has " << f_udp_buffers.size() << " ports open" );

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

        LDEBUG( plog, "Closing port <" << a_port << ">" );

        t_it->second->f_iterator.release();
        delete t_it->second;
        f_udp_buffers.erase( t_it );

        LDEBUG( plog, "Port <" << a_port << "> is closed; distributor now has " << f_udp_buffers.size() << " ports open" );

        return true;
    }

    void packet_distributor::initialize()
    {
        LDEBUG( plog, "Initializing packet distributor" );
        if( ! f_ip_pkt_iterator.attach( f_ip_buffer ) )
        {
            throw error() << "[packet_distributor] Unable to attach iterator to buffer";
        }
        return;
    }

    void packet_distributor::execute()
    {
        LDEBUG( plog, "Packet distributor is starting execution" );

        // move the read iterator up until it runs up against a blocked packet
        while( +f_ip_pkt_iterator );
        f_ip_pkt_iterator.set_unused();

        // set write packet statuses
        for( buffer_map::iterator t_it = f_udp_buffers.begin(); t_it != f_udp_buffers.end(); ++t_it )
        {
            t_it->second->f_iterator.set_writing();
        }

        while( ! is_canceled() )
        {
            // advance the iterator; blocks until next packet is available
            LDEBUG( plog, "Attempting to advance IP packet read iterator" );
            ++f_ip_pkt_iterator;

            // check for cancelation
            if( is_canceled() )
            {
                LINFO( plog, "Packet distributor was canceled (2); exiting" );
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

        LINFO( plog, "Packet distributor was canceled (1); exiting" );
        return;
    }

    bool packet_distributor::distribute_packet()
    {
        // this doesn't appear to be defined anywhere in standard linux headers, at least as far as I could find
        static const unsigned t_udp_hdr_len = 8;

        LDEBUG( plog, "Reading IP packet at iterator position " << f_ip_pkt_iterator.index() );

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

        t_it->second->f_iterator.set_writing();

        // copy the UPD packet from the IP packet into the appropriate buffer
        t_it->second->f_iterator->memcpy( reinterpret_cast< uint8_t* >( t_udp_hdr + t_udp_hdr_len ), htons(t_udp_hdr->len) - t_udp_hdr_len );

        t_it->second->f_iterator.set_written();

        return true;
    }

} /* namespace psyllid */
