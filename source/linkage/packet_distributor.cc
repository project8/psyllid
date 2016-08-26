/*
 * packet_distributor.cc
 *
 *  Created on: Aug 23, 2016
 *      Author: nsoblath
 */

#include "packet_distributor.hh"

#include "logger.hh"

namespace psyllid
{
    LOGGER( plog, "packet_distributor" );


    packet_distributor::packet_distributor() :
            f_ip_pkt_iterator(),
            f_udp_buffers()
    {
    }

    packet_distributor::~packet_distributor()
    {
    }

    bool packet_distributor::open_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size )
    {
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

    void packet_distributor::execute()
    {

        while( ! is_canceled() )
        {

        }


    }

} /* namespace psyllid */
