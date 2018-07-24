/*
 * packet_receiver_fpa.cc
 *
 *  Created on: Nov 2, 2016
 *      Author: nsoblath
 */

#include "packet_receiver_fpa.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
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

// debug
#ifndef NDEBUG
#include <netinet/ether.h>
#endif

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( packet_receiver_fpa, "packet-receiver-fpa", packet_receiver_fpa_binding );

    LOGGER( plog, "packet_receiver_fpa" );

    packet_receiver_fpa::packet_receiver_fpa() :
            f_length( 10 ),
            f_max_packet_size( 1048576 ),
            f_port( 23530 ),
            f_interface( "eth1" ),
            f_timeout_sec( 1 ),
            f_n_blocks( 64 ),
            f_block_size( 1 << 22 ),
            f_frame_size( 1 << 14 ),
            f_net_interface_index( 0 ),
            f_socket( 0 ),
            f_ring(),
            f_packets_total( 0 ),
            f_bytes_total( 0 )
    {
    }

    packet_receiver_fpa::~packet_receiver_fpa()
    {
        cleanup_fpa();
    }

    void packet_receiver_fpa::initialize()
    {
        out_buffer< 0 >().initialize( f_length );

        f_net_interface_index = if_nametoindex( f_interface.c_str() );
        if( f_net_interface_index == 0 )
        {
            throw error() << "[packet_receiver_fpa] Unable to find index for interface <" << f_interface << ">";
        }
        LDEBUG( plog, "Identified net interface <" << f_interface << "> with index <" << f_net_interface_index << ">" );

        LDEBUG( plog, "Preparing fast-packet-acquisition on interface " << f_interface << " at UDP port " << f_port );

        // open socket
        //int fd;
        f_socket = ::socket( AF_PACKET, SOCK_RAW, htons(ETH_P_IP) );
        if( f_socket < 0 )
        {
            throw error() << "Could not create socket:\n\t" << strerror( errno );
        }

        int t_packet_ver = TPACKET_V3;
        if( ::setsockopt( f_socket, SOL_PACKET, PACKET_VERSION, &t_packet_ver, sizeof(int) ) < 0 )
        {
            throw error() << "Could not set packet version:\n\t" << strerror( errno );
        }

        // create the ring buffer
        //f_ring = new receive_ring();
        //::memset( f_ring, 0, sizeof(receive_ring) );
        //::memset( &f_ring->f_req, 0, sizeof(tpacket_req3) );
        LDEBUG( plog, "Ring buffer parameters:\n" <<
                "block size: " << f_block_size << '\n' <<
                "frame size: " << f_frame_size << '\n' <<
                "number of blocks: " << f_n_blocks );
        f_ring.f_req.tp_block_size = f_block_size;
        f_ring.f_req.tp_frame_size = f_frame_size;
        f_ring.f_req.tp_block_nr = f_n_blocks;
        f_ring.f_req.tp_frame_nr = (f_block_size * f_n_blocks) / f_frame_size;
        f_ring.f_req.tp_retire_blk_tov = 60;
        f_ring.f_req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

        //LWARN( plog, "f_ring = " << f_ring );
#ifndef NDEBUG
        bool test = f_ring.f_rd == nullptr;
        LTRACE( plog, "f_ring.f_rd == nullptr: " << test );
        test = f_ring.f_map == nullptr;
        LTRACE( plog, "f_ring.f_map == nullptr: " << test );
#endif
        LTRACE( plog, "f_ring.f_req.tp_block_size = " << f_ring.f_req.tp_block_size );

        LDEBUG( plog, "Opening packet_eater for network interface <" << f_interface << ">" );

        LTRACE( plog, "f_socket = " << f_socket << ";  SOL_PACKET = " << SOL_PACKET << ";  PACKET_RX_RING = " << PACKET_RX_RING << ";  &f_ring.f_req = " << &f_ring.f_req << ";  sizeof(f_ring.f_req) = " << sizeof(f_ring.f_req) );
        if( ::setsockopt( f_socket, SOL_PACKET, PACKET_RX_RING, &f_ring.f_req, sizeof(f_ring.f_req) ) < 0 )
        {
            throw error() << "Could not set receive ring:\n\t" << strerror( errno );
        }

        /* setsockopt: Handy debugging trick that lets
           * us rerun the udp_server immediately after we kill it;
           * otherwise we have to wait about 20 secs.
           * Eliminates "ERROR on binding: Address already in use" error.
           */
        //int optval = 1;
        //::setsockopt( f_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

        // Receive timeout
        if( f_timeout_sec > 0 )
        {
            timeval t_timeout;
            t_timeout.tv_sec = f_timeout_sec;
            t_timeout.tv_usec = 0;  // Not init'ing this can cause strange errors
            ::setsockopt( f_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_timeout, sizeof(struct timeval) );
        }

        // finish preparing the ring
        f_ring.f_map = (uint8_t*)::mmap( nullptr, f_ring.f_req.tp_block_size * f_ring.f_req.tp_block_nr,
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, f_socket, 0);
        if( f_ring.f_map == MAP_FAILED )
        {
            throw error() << "Unable to setup ring map";
        }

        f_ring.f_rd = (iovec*)::malloc(f_ring.f_req.tp_block_nr * sizeof(*f_ring.f_rd) );
        if( f_ring.f_rd == nullptr )
        {
            throw error() << "Unable to allocate memory for the ring";
        }
        for( unsigned i_block = 0; i_block < f_ring.f_req.tp_block_nr; ++i_block )
        {
            f_ring.f_rd[ i_block ].iov_base = f_ring.f_map + ( i_block * f_ring.f_req.tp_block_size );
            f_ring.f_rd[ i_block ].iov_len = f_ring.f_req.tp_block_size;
        }

        // initialize the address
        sockaddr_ll t_address;// = new sockaddr_ll();
        ::memset( &t_address, 0, sizeof(sockaddr_ll) );
        t_address.sll_family = PF_PACKET;
        t_address.sll_protocol = htons(ETH_P_IP);
        t_address.sll_ifindex = f_net_interface_index;
        t_address.sll_hatype = 0;
        t_address.sll_pkttype = 0;
        t_address.sll_halen = 0;

        if( ::bind( f_socket, (sockaddr*)&t_address, sizeof(t_address) ) < 0 )
        {
            throw error() << "Could not bind socket:\n\t" << strerror( errno );
        }

        LINFO( plog, "Ready to consume packets on interface <" << f_interface << ">" );

        return;
    }

    void packet_receiver_fpa::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the packet_receiver_fpa" );

            //memory_block* t_mem_block = nullptr;

            // Setup the polling file descriptor struct
            pollfd t_pollfd;
            ::memset( &t_pollfd, 0, sizeof(pollfd) );
            t_pollfd.fd = f_socket;
            t_pollfd.events = POLLIN | POLLERR;
            t_pollfd.revents = 0;

            unsigned t_block_num = 0;
            block_desc* t_block = nullptr;

            unsigned t_timeout_msec = 1000 * f_timeout_sec;

            //LDEBUG( plog, "Server is listening" );

            std::unique_ptr< char[] > t_buffer_ptr( new char[ f_max_packet_size ] );

            if( ! out_stream< 0 >().set( stream::s_start ) ) return;

            LINFO( plog, "Starting main loop; waiting for packets" );
            while( ! is_canceled() )
            {

                if( (out_stream< 0 >().get() == stream::s_stop) )
                {
                    LWARN( plog, "Output stream(s) have stop condition" );
                    break;
                }

                // get the next block
                t_block = (block_desc *) f_ring.f_rd[ t_block_num ].iov_base;

                // make sure the next block has been made available to the user
                if( ( t_block->f_packet_hdr.block_status & TP_STATUS_USER ) == 0 )
                {
                    // next block isn't available yet, so poll until it is, with the specified timeout
                    // timeout or successful poll will go back to the top of the loop
                    poll( &t_pollfd, 1, t_timeout_msec );
                    continue;
                }

                // we have a block available, so process it
                unsigned t_num_pkts = t_block->f_packet_hdr.num_pkts;
                unsigned long t_bytes = 0;

                tpacket3_hdr* t_packet = reinterpret_cast< tpacket3_hdr* >( (uint8_t*)t_block + t_block->f_packet_hdr.offset_to_first_pkt );

                LTRACE( plog, "Walking a block with " << t_num_pkts << " packets" );
                for( unsigned i = 0; i < t_num_pkts; ++i )
                {
                    t_bytes += t_packet->tp_snaplen;

                    LTRACE( plog, "Processing IP packet; current iterator index is <" << out_stream< 0 >().get_current_index() << ">" );
                    if( process_packet( t_packet ) )
                    {
                        LTRACE( plog, "UDP packet processed; outputing to stream index <" << out_stream< 0 >().get_current_index() << ">" );
                        if( ! out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            break;
                        }
                    }
                    else
                    {
                        LTRACE( plog, "Packet was not processed properly; skipping" );
                    }

                    // update the address of the packet to the next packet in the block
                    t_packet = reinterpret_cast< tpacket3_hdr* >( (uint8_t*)t_packet + t_packet->tp_next_offset );
                }
                LTRACE( plog, "Done walking block" );

                f_packets_total += t_num_pkts;
                f_bytes_total += t_bytes;

                // return the block to the kernel; we're done with it
                t_block->f_packet_hdr.block_status = TP_STATUS_KERNEL;
                t_block_num = ( t_block_num + 1 ) % f_n_blocks;

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
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    bool packet_receiver_fpa::process_packet( tpacket3_hdr* a_packet )
    {
        //printf("rxhash: 0x%x\n", a_packet->hv1.tp_rxhash);

        //****************
        // Ethernet Packet
        //****************

        // grab the ethernet interface header (defined in if_ether.h)
        ethhdr* t_eth_hdr = reinterpret_cast< ethhdr* >( (uint8_t*)a_packet + a_packet->tp_mac );

#ifndef NDEBUG
        char t_macstr_dest[3*ETH_ALEN], t_macstr_source[3*ETH_ALEN];
        ether_ntoa_r((struct ether_addr*)&(t_eth_hdr->h_dest), t_macstr_dest);
        ether_ntoa_r((struct ether_addr*)&(t_eth_hdr->h_source), t_macstr_source);
        LTRACE( plog, "ethhdr: h_dest: " << t_macstr_dest << ";  h_source: " << t_macstr_source << ";  h_proto: " << ntohs(t_eth_hdr->h_proto) );
#endif
        //LWARN( plog, "Ethernet header: " << t_eth_p_ip << "; htons(ETH_P_IP): " << htons(ETH_P_IP) << "; t_eth_hdr->h_proto: " << t_eth_hdr->h_proto );
        //for (int i=0; i<50; ++i)
        //{
        //    printf("%02X", ((char*)a_packet)[i]);
        //}
        //printf("\n");

        LTRACE( plog, "Ethernet sizes (total, header, data): ???, " << ETH_HLEN << ", ???" );
        LTRACE( plog, "Ethernet mem addresses (packet/header, data): " << t_eth_hdr << ", " << (void*)( (char*)t_eth_hdr + ETH_HLEN ) );

        // filter only IP packets
        static const unsigned short t_eth_p_ip = htons(ETH_P_IP);
        if( t_eth_hdr->h_proto != t_eth_p_ip )
        {
            LDEBUG( plog, "Non-IP packet skipped" );
            return false;
        }


        //**********
        // IP Packet
        //**********

        //LWARN( plog, "Handling IP packet" );
        // grab the ip interface header (defined in ip.h)
        iphdr* t_ip_hdr = reinterpret_cast< iphdr* >( (char*)t_eth_hdr + ETH_HLEN );

#ifndef NDEBUG
        char t_source_ip[16], t_dest_ip[16];
        inet_ntop(AF_INET, &t_ip_hdr->saddr, t_source_ip, 16);
        inet_ntop(AF_INET, &t_ip_hdr->daddr, t_dest_ip, 16);
        LTRACE( plog, "IP header: version: " << unsigned(t_ip_hdr->version) << ";  ihl: " << unsigned(t_ip_hdr->ihl) << ";  tos: " << ntohs(t_ip_hdr->tos) << ";  tot_len: " << ntohs(t_ip_hdr->tot_len) << ";  protocol: " << unsigned(t_ip_hdr->protocol) << ";  saddr: " << t_source_ip << ";  daddr: " << t_dest_ip );
#endif

        //TODO: filter on source address?
        //uint32_t t_source_address = t_ip_hdr->saddr;

        // get ip packet data
        //char* t_ip_data = (char*)t_ip_hdr + t_ip_hdr->ihl * 4;
        //size_t t_ip_data_len = (uint8_t)htons(t_ip_hdr->tot_len) - t_ip_header_bytes;

        LTRACE( plog, "IP sizes (total, header, data): " << ntohs(t_ip_hdr->tot_len) << ", " << unsigned(t_ip_hdr->ihl * 4) << ", " << ntohs(t_ip_hdr->tot_len) - t_ip_hdr->ihl * 4 );
        LTRACE( plog, "IP mem addresses(packet/header, data): " << t_ip_hdr << ", " << (void*)( (char*)t_ip_hdr + t_ip_hdr->ihl * 4 ) );

        if( t_ip_hdr->protocol != 17 )
        {
            LDEBUG( plog, "Non-UDP packet skipped" );
            return false;
        }

        //***********
        // UDP Packet
        //***********

        // this doesn't appear to be defined anywhere in standard linux headers, at least as far as I could find
        static const unsigned t_udp_hdr_len = sizeof( udphdr );

        udphdr* t_udp_hdr = reinterpret_cast< udphdr* >( (char*)t_ip_hdr + t_ip_hdr->ihl * 4 );

        LTRACE( plog, "UDP header: source port: " << ntohs(t_udp_hdr->source) << ";  dest port: " << ntohs(t_udp_hdr->dest) << ";  len: " << ntohs(t_udp_hdr->len) << ";  check: " << ntohs(t_udp_hdr->check) );

        // get port number
        //unsigned t_port = ntohs(t_udp_hdr->dest);

        // check port number against configured port
        if( ntohs(t_udp_hdr->dest) != f_port )
        {
            LDEBUG( plog, "Destination port is incorrect: expected " << f_port << " but got " << ntohs(t_udp_hdr->dest) );
            return false;
        }

        size_t t_udp_data_len = ntohs(t_udp_hdr->len) - t_udp_hdr_len;

        LTRACE( plog, "UDP sizes (total, header, data): " << ntohs(t_udp_hdr->len) << ", " << t_udp_hdr_len << ", " << t_udp_data_len );
        LTRACE( plog, "UDP mem addresses (packet/header, data): " << t_udp_hdr << ", " << (void*)((char*)t_udp_hdr + t_udp_hdr_len) );

        memory_block* t_mem_block = out_stream< 0 >().data();
        t_mem_block->resize( f_max_packet_size );

        LTRACE( plog, "Packet received (" << t_udp_data_len << " bytes); block address is " << (void*)t_mem_block->block() );

        LTRACE( plog, "Packet words: " << std::hex << strtoull((char*)t_mem_block->block(), NULL, 0) );
        LTRACE( plog, "Packet bytes: " << unsigned(((char*)t_mem_block->block())[0]) << " " << unsigned(((char*)t_mem_block->block())[1]) << " " << unsigned(((char*)t_mem_block->block())[2]) );

        // copy the UPD packet from the IP packet into the appropriate buffer
        //uint8_t* t_udp_data = (uint8_t*)t_udp_hdr + t_udp_hdr_len;
        ::memcpy( reinterpret_cast< void* >( t_mem_block->block() ),
                  reinterpret_cast< void* >( (char*)t_udp_hdr + t_udp_hdr_len ),
                  t_udp_data_len );
        t_mem_block->set_n_bytes_used( t_udp_data_len );

        return true;
    }

    void packet_receiver_fpa::finalize()
    {
        out_buffer< 0 >().finalize();

        LINFO( plog, "Turning off the FPA" );
        cleanup_fpa();

        return;
    }

    void packet_receiver_fpa::cleanup_fpa()
    {
        if( f_ring.f_map != nullptr )
        {
            LDEBUG( plog, "Unmapping mmap ring" );
            ::munmap(f_ring.f_map, f_ring.f_req.tp_block_size * f_ring.f_req.tp_block_nr);
            f_ring.f_map = nullptr;
        }
        if( f_ring.f_rd != nullptr )
        {
            LDEBUG( plog, "freeing f_rd" );
            ::free( f_ring.f_rd );
            f_ring.f_rd = nullptr;
        }

        // close socket
        if( f_socket != 0 )
        {
            ::close( f_socket );
            f_socket = 0;
        }

        return;
    }



    packet_receiver_fpa_binding::packet_receiver_fpa_binding() :
            _node_binding< packet_receiver_fpa, packet_receiver_fpa_binding >()
    {
    }

    packet_receiver_fpa_binding::~packet_receiver_fpa_binding()
    {
    }

    void packet_receiver_fpa_binding::do_apply_config( packet_receiver_fpa* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring packet_receiver_fpa with:\n" << a_config );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        a_node->set_port( a_config.get_value( "port", a_node->get_port() ) );
        a_node->interface() = a_config.get_value( "interface", a_node->interface() );
        a_node->set_timeout_sec( a_config.get_value( "timeout-sec", a_node->get_timeout_sec() ) );
        a_node->set_n_blocks( a_config.get_value( "n-blocks", a_node->get_n_blocks() ) );
        a_node->set_block_size( a_config.get_value( "block-size", a_node->get_block_size() ) );
        a_node->set_frame_size( a_config.get_value( "frame-size", a_node->get_frame_size() ) );
        return;
    }

    void packet_receiver_fpa_binding::do_dump_config( const packet_receiver_fpa* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for packet_receiver_fpa" );
        a_config.add( "length", scarab::param_value( a_node->get_length() ) );
        a_config.add( "port", scarab::param_value( a_node->get_port() ) );
        a_config.add( "interface", scarab::param_value( a_node->interface() ) );
        a_config.add( "timeout-sec", scarab::param_value( a_node->get_timeout_sec() ) );
        a_config.add( "n-blocks", scarab::param_value( a_node->get_n_blocks() ) );
        a_config.add( "block-size", scarab::param_value( a_node->get_block_size() ) );
        a_config.add( "frame-size", scarab::param_value( a_node->get_frame_size() ) );
        return;
    }

} /* namespace psyllid */
