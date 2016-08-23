/*
 * packet_eater.cc
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#include "packet_eater.hh"

#include "packet_buffer.hh"

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
	        f_timeout_sec( a_timeout_sec ),
			f_socket( 0 ),
			f_ring( nullptr ),
			f_address( nullptr ),
			f_packets_total( 0 ),
			f_bytes_total( 0 ),
			f_packet_buffer( 100, 0 ),
			f_iterator()

	{
        LINFO( plog, "Preparing fast-packet-acquisition server" );

        // create the ring buffer
        f_ring = new receive_ring();
        ::memset( f_ring, 0, sizeof(receive_ring) );
        unsigned t_block_size = 1 << 22;
        unsigned t_frame_size = 1 << 11;
        unsigned t_block_num = 64;
        LDEBUG( plog, "Ring buffer parameters: block size: " << t_block_size << "; frame size: " << t_frame_size << "; block number: " << t_block_number );
        ::memset( &f_ring->f_req, 0, sizeof(tpacket_req3) );
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
        f_ring->f_map = ::mmap( nullptr, f_ring->f_req.tp_block_size * f_ring->f_req.tp_block_nr,
        		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, f_socket, 0);
        if( f_ring->f_map == MAP_FAILED )
        {
        	throw midge::error() << "[udp_server] Unable to setup ring map";
        	return;
        }

        f_ring->f_rd = ::malloc(f_ring->f_req.tp_block_nr * sizeof(*f_ring->f_rd) );
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
        f_address = new sockaddr_ll();
        ::memset( f_address, 0, sizeof(sockaddr_ll) );
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
        LDEBUG( plog, "Binding the socket" );
        if( ::bind( f_socket, (const sockaddr*)f_address, sizeof(f_address) ) < 0 )
        {
        	throw midge::error() << "[udp_server] could not bind socket:\n\t" << strerror( errno );
        	return;
        }

        LINFO( plog, "Ready to receive messages on port " << a_port );

        // create iterator for packet buffer
        f_iterator.attach( &f_packet_buffer );

        return;
	}

	packet_eater::~packet_eater()
	{
	    // undo the ring
        ::munmap(f_ring->f_map, f_ring->f_req.tp_block_size * f_ring->f_req.tp_block_nr);
        ::free( f_ring->f_rd );

        // clean up socket address
        delete f_address;

        // close udp_server socket
        ::close( f_socket );

        // delete the packet buffer(s)
        while( ! f_packet_buffers.empty() )
        {
            delete f_packet_buffers.front();
            f_packet_buffers.pop_front();
        }
	}

	void packet_eater::execute()
	{
	    // Setup the polling file descriptor struct
	    pollfd t_pollfd;
	    ::memset( &t_pollfd, 0, sizeof(pollfd) );
	    t_pollfd.fd = f_socket;
	    t_pollfd.events = POLLIN | POLLERR;
	    t_pollfd.revents = 0;

	    unsigned t_block_num = 0;
	    block_desc* t_pdb = nullptr;

	    unsigned t_timeout_msec = 1000 * f_timeout_sec;

	    while( ! is_canceled() )
	    {
	        t_pdb = (block_desc *) f_ring->f_rd[ t_block_num ].iov_base;

	        if( ( t_pbd->f_h1.block_status & TP_STATUS_USER ) == 0 )
	        {
	            poll( &t_pollfd, 1, t_timeout_msec );
	            continue;
	        }

            walk_block(pbd, block_num);
            flush_block(pbd);
            block_num = (block_num + 1) % blocks;
        }


	}

    void packet_eater::walk_block( block_desc* a_bd, const int a_block_num )
    {
        unsigned t_num_pkts = a_bd->f_h1.num_pkts;
        unsigned long t_bytes = 0;

        tpacket3_hdr* t_packet = reinterpret_cast< tpacket3_hdr* >( (uint8_t*)a_bd + a_bd->f_h1.offset_to_first_pkt );

        for( unsigned i = 0; i < t_num_pkts; ++i )
        {
            t_bytes += t_packet->tp_snaplen;

            process_packet( t_packet );

            // move packet iterator ahead if possible
            if( +f_iterator == false )
            {
                LINFO( plog, "Packet iterator blocked at <" << f_iterator.index() << ">" );
                //increment block (waits for mutex lock)
                ++f_iterator;
                LINFO( plog, "Packet iterator loose at <" << f_iterator.index() << ">" );
            }

            // update the address of the packet to the next packet in the block
            t_packet = reinterpret_cast< tpacket3_hdr* >( (uint8_t*)a_bd + t_packet->tp_next_offset );
        }

        f_packets_total += t_num_pkts;
        f_bytes_total += t_bytes;

        return;
    }

    void packet_eater::flush_block( block_desc* a_bd )
    {
        a_bd->f_h1.block_status = TP_STATUS_KERNEL;
        return;
    }

    void packet_eater::process_packet( tpacket3_hdr* a_packet )
    {
        // grab the ethernet interface header (defined in if_ether.h)
        ethhdr* t_eth_hdr = reinterpret_cast< ethhdr* >( (uint8_t*)a_packet + a_packet->tp_mac );

        // filter only IP packets
        static const unsigned short t_eth_p_ip = htons(ETH_P_IP);
        if( t_eth_hdr->h_proto != t_eth_p_ip )
        {
            LDEBUG( plog, "Non-IP packet skipped" );
            return;
        }

        // grab the ip interface header (defined in ip.h)
        iphdr* t_ip_hdr = reinterpret_cast< iphdr* >( (uint8_t*)t_eth_hdr + ETH_HLEN );

        //TODO: filter on source address?
        //uint32_t t_source_address = t_ip_hdr->saddr;

        //TODO: use destination address for routing to different channels?
        //uint32_t t_dest_address = t_ip_hdr->daddr;

        //TODO: check inputs to copy function
        f_iterator->copy( reinterpret_cast< uint8_t* >( (uint8_t*)t_ip_hdr + t_ip_hdr->ihl ), t_ip_hdr->tot_len - t_ip_hdr->ihl);

        //TODO: put packet into packet buffer
        //TODO: (later) check channel, and put into the proper buffer

    }


} /* namespace psyllid */
