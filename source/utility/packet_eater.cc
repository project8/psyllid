/*
 * packet_eater.cc
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#include "packet_eater.hh"

#include "logger.hh"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <net/if.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>



LOGGER( plog, "udp_server" );


namespace psyllid {

	packet_eater::packet_eater( unsigned a_timeout_sec ) :
			f_socket( 0 ),
			f_ring( nullptr ),
			f_address( nullptr )

	{
        LINFO( plog, "Preparing fast-packet-acquisition server" );

        // create the ring buffer
        unsigned t_block_size = 1 << 22;
        unsigned t_frame_size = 1 << 11;
        unsigned t_block_num = 64;
        LDEBUG( plog, "Ring buffer parameters: block size: " << t_block_size << "; frame size: " << t_frame_size << "; block number: " << t_block_number );
        ::memset( &f_ring->f_req, 0, sizeof(f_ring->f_req) );
        f_ring->f_req.tp_block_size = t_block_size;
        f_ring->f_req.tp_frame_size = t_frame_size;
        f_ring->f_req.tp_block_nr = t_block_num;
        f_ring->f_req.tp_frame_nr = (t_block_size - t_block_num) / t_frame_size;
        f_ring->f_req.tp_retire_blk_tov = 60;
        f_ring->f_req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

        LDEBUG( plog, "Opening udp_server socket on port <" << a_port << ">" );

        // open socket
        f_socket = ::socket( AF_PACKET, SOCK_RAW, htons(ETH_P_IP) );
        if( f_socket < 0 )
        {
            throw midge::error() << "[udp_server] could not create socket:\n\t" << strerror( errno );
            return;
        }

        // socket options

        int t_packet_ver = TPACKET_V3;
        if( ::setsockopt( f_socket, SOL_PACKET, PACKET_VERSION, &t_packet_ver, sizeof(t_packet_ver) ) < 0 )
        {
        	throw midge::error() << "[udp_server] could not set packet version:\n\t" << strerror( errno );
        	return;
        }

        if( ::setsockopt( f_socket, SOL_PACKET, PACKET_RX_RING, &f_ring->f_req, sizeof(f_ring->f_req) ) < 0 )
        {
        	throw midge::error() << "[udp_server] could not set receive ring:\n\t" << strerror( errno );
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

        // finish preparing the ring
        f_ring->f_map = mmap( nullptr, f_ring->f_req.tp_block_size * f_ring->f_req.tp_block_nr,
        		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, f_socket, 0);
        if( f_ring->f_map == MAP_FAILED )
        {
        	throw midge::error() << "[udp_server] Unable to setup ring map";
        	return;
        }

        f_ring->f_rd = malloc(f_ring->f_req.tp_block_nr * sizeof(*f_ring->f_rd) );
        if( f_ring->f_rd == nullptr )
        {
        	throw midge::error() << "[udp_server] Unable to allocate memory for the ring";
        	return;
        }
        for( unsigned i_block = 0; i_block < f_ring->f_req.tp_block_nr; ++i_block )
        {
        	f_ring->f_rd[ i_block ].iov_base = f_ring->f_map + ( i_block * f_ring->f_req.tp_block_size );
        	f_ring->f_rd[ i_block ].iov_len = f_ring->f_req.tp_block_size;
        }

        // initialize the address
        ::memset( &f_address, 0, sizeof(f_address) );
        int t_interface_index = if_nametoindex( a_interface );
        if( t_interface_index == 0 )
        {
        	throw midge::error() << "[udp_server] Unable to find index for interface <" << a_interface << ">";
        	return;
        }
        f_address->sll_family = PF_PACKET;
        f_address->sll_protocol = htons(ETH_P_IP);
        f_address->sll_ifindex = t_interface_index;
        f_address->sll_hatype = 0;
        f_address->sll_pkttype = 0;
        f_address->sll_halen = 0;

        // bind the socket
        DEBUG( plog, "Binding the socket" );
        if( ::bind( f_socket, (const sockaddr*)f_address, sizeof(f_address) ) < 0 )
        {
        	throw midge::error() << "[udp_server] could not bind socket:\n\t" << strerror( errno );
        	return;
        }

        LINFO( plog, "Ready to receive messages on port " << a_port );

        return;	}

	packet_eater::~packet_eater()
	{
        //clean up udp_server address
        delete f_address;

        //close udp_server socket
        ::close( f_socket );
	}

} /* namespace psyllid */
