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

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( packet_receiver_fpa, "packet-receiver-socket" );

    LOGGER( plog, "packet_receiver_fpa" );

    packet_receiver_fpa::packet_receiver_fpa() :
            f_length( 10 ),
            f_max_packet_size( 1048576 ),
            f_port( 23530 ),
            f_interface( "eth1" ),
            f_timeout_sec( 1 ),
            f_timeout_sec( 1 ),
            f_n_blocks( 64 ),
            f_block_size( 1 << 22 ),
            f_frame_size( 1 << 11 ),
            f_net_interface_index( 0 ),
            f_socket( 0 ),
            f_ring(),
            f_packets_total( 0 ),
            f_bytes_total( 0 )
    {
    }

    packet_receiver_fpa::~packet_receiver_fpa()
    {
        // undo the ring
        LWARN( plog, "f_ring->f_rd = " << f_ring.f_rd );
        LWARN( plog, "f_ring->f_map = " << f_ring.f_map );
        LWARN( plog, "f_ring->f_req.tp_block_size = " << f_ring.f_req.tp_block_size );

        ::munmap(f_ring.f_map, f_ring.f_req.tp_block_size * f_ring.f_req.tp_block_nr);
        if( f_ring.f_rd != nullptr )
        {
            LERROR( plog, "freeing f_rd" );
            ::free( f_ring.f_rd );
        }

        // close socket
        if( f_socket > 0 ) ::close( f_socket );
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
            LERROR( plog, "Could not create socket:\n\t" << strerror( errno ) );
            return false;
        }

        int t_packet_ver = TPACKET_V3;
        if( ::setsockopt( f_socket, SOL_PACKET, PACKET_VERSION, &t_packet_ver, sizeof(int) ) < 0 )
        {
            LERROR( plog, "Could not set packet version:\n\t" << strerror( errno ) );
            return false;
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
        bool test = f_ring.f_rd == nullptr;
        LWARN( plog, "f_ring.f_rd == nullptr: " << test );
        test = f_ring.f_map == nullptr;
        LWARN( plog, "f_ring.f_map == nullptr: " << test );
        LWARN( plog, "f_ring.f_req.tp_block_size = " << f_ring.f_req.tp_block_size );

        LDEBUG( plog, "Opening packet_eater for network interface <" << f_net_interface_name << ">" );

        LWARN( plog, "f_socket = " << f_socket << ";  SOL_PACKET = " << SOL_PACKET << ";  PACKET_RX_RING = " << PACKET_RX_RING << ";  &f_ring.f_req = " << &f_ring.f_req << ";  sizeof(f_ring.f_req) = " << sizeof(f_ring.f_req) );
        if( ::setsockopt( f_socket, SOL_PACKET, PACKET_RX_RING, &f_ring.f_req, sizeof(f_ring.f_req) ) < 0 )
        {
            LERROR( plog, "Could not set receive ring:\n\t" << strerror( errno ) );
            return false;
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
            LERROR( plog, "Unable to setup ring map" );
            return false;
        }

        f_ring.f_rd = (iovec*)::malloc(f_ring.f_req.tp_block_nr * sizeof(*f_ring.f_rd) );
        if( f_ring.f_rd == nullptr )
        {
            LERROR( plog, "Unable to allocate memory for the ring" );
            return false;
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
            LERROR( plog, "Could not bind socket:\n\t" << strerror( errno ) );
            return false;
        }

        LINFO( plog, "Ready to consume packets on interface <" << f_interface << ">" );

        return;
    }

    void packet_receiver_fpa::execute()
    {
        LDEBUG( plog, "Executing the packet_receiver_fpa" );

        memory_block* t_block = nullptr;

        // Setup the polling file descriptor struct
        pollfd t_pollfd;
        ::memset( &t_pollfd, 0, sizeof(pollfd) );
        t_pollfd.fd = f_socket;
        t_pollfd.events = POLLIN | POLLERR;
        t_pollfd.revents = 0;

        unsigned t_block_num = 0;
        block_desc* t_block = nullptr;

        unsigned t_timeout_msec = 1000 * f_timeout_sec;

        try
        {
            //LDEBUG( plog, "Server is listening" );

            std::unique_ptr< char[] > t_buffer_ptr( new char[ f_max_packet_size ] );

            out_stream< 0 >().set( stream::s_start );

            ssize_t t_size_received = 0;

            LINFO( plog, "Starting main loop; waiting for packets" );
            while( ! f_canceled.load() )
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

                LDEBUG( plog, "Walking a block with " << t_num_pkts << " packets" );
                for( unsigned i = 0; i < t_num_pkts; ++i )
                {
                    t_bytes += t_packet->tp_snaplen;

                    LDEBUG( plog, "Attempting to write IP packet at iterator index " << out_stream< 0 >().get_current_index() );
                    if( process_packet( t_packet ) )
                    {
                        out_stream< 0 >().set( stream::s_run );
                    }

                    // update the address of the packet to the next packet in the block
                    t_packet = reinterpret_cast< tpacket3_hdr* >( (uint8_t*)t_packet + t_packet->tp_next_offset );
                }
                LDEBUG( plog, "Done walking block" );

                f_packets_total += t_num_pkts;
                f_bytes_total += t_bytes;

                // return the block to the kernel; we're done with it
                t_block->f_packet_hdr.block_status = TP_STATUS_KERNEL;
                t_block_num = ( t_block_num + 1 ) % f_n_blocks;

            }

            LINFO( plog, "Packet receiver is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output streams" );
            out_stream< 0 >().set( stream::s_stop );

            LDEBUG( plog, "Exiting output streams" );
            out_stream< 0 >().set( stream::s_exit );

            return;
        }
        catch( midge::error& e )
        {
            LERROR( plog, "Midge exception caught: " << e.what() );

            LDEBUG( plog, "Stopping output stream" );
            out_stream< 0 >().set( stream::s_stop );

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

            return;
        }
        catch( error& e )
        {
            LERROR( plog, "Psyllid exception caught: " << e.what() );

            LDEBUG( plog, "Stopping output stream" );
            out_stream< 0 >().set( stream::s_stop );

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

            return;
        }

        // control should not reach here
        LERROR( plog, "Control should not reach this point" );
        return;
    }

    bool packet_receiver_fpa::process_packet( tpacket3_hdr* a_packet )
    {
        //****************
        // Ethernet Packet
        //****************

        // grab the ethernet interface header (defined in if_ether.h)
        ethhdr* t_eth_hdr = reinterpret_cast< ethhdr* >( (uint8_t*)a_packet + a_packet->tp_mac );

        // filter only IP packets
        static const unsigned short t_eth_p_ip = htons(ETH_P_IP);
        //LWARN( plog, "t_eth_p_ip: " << t_eth_p_ip << "; htons(ETH_P_IP): " << htons(ETH_P_IP) << "; t_eth_hdr->h_proto: " << t_eth_hdr->h_proto );
        //for (int i=0; i<50; ++i)
        //{
        //    printf("%02X", ((char*)a_packet)[i]);
        //}
        //printf("\n");

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
        iphdr* t_ip_hdr = reinterpret_cast< iphdr* >( (uint8_t*)t_eth_hdr + ETH_HLEN );

        //TODO: filter on source address?
        //uint32_t t_source_address = t_ip_hdr->saddr;

        // get ip packet data
        uint8_t t_ip_header_bytes = t_ip_hdr->ihl * 4;
        uint8_t* t_ip_data = (uint8_t*)t_ip_hdr + t_ip_header_bytes;
        //size_t t_ip_data_len = (uint8_t)htons(t_ip_hdr->tot_len) - t_ip_header_bytes;


        //***********
        // UDP Packet
        //***********

        // this doesn't appear to be defined anywhere in standard linux headers, at least as far as I could find
        static const unsigned t_udp_hdr_len = 8;

        udphdr* t_udp_hdr = reinterpret_cast< udphdr* >( t_ip_data );

        // get port number
        unsigned t_port = htons(t_udp_hdr->dest);

        // check port number against configured port
        if( t_port != f_port ) return false;

        // copy the UPD packet from the IP packet into the appropriate buffer
        uint8_t* t_udp_data = t_udp_hdr + t_udp_hdr_len;
        size_t t_udp_data_len = htons(t_udp_hdr->len) - t_udp_hdr_len;

        t_block = out_stream< 0 >().data();
        ::memcpy( &t_block->get_block(), t_udp_data, t_udp_data_len );

        LDEBUG( plog, "Packet received (" << t_udp_data_len << " bytes)" );
        LDEBUG( plog, "Packet written to stream index <" << out_stream< 0 >().get_current_index() << ">" );

        out_stream< 0 >().set( stream::s_run );

        return true;
    }

    void packet_receiver_fpa::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }

    void packet_receiver_fpa::do_cancellation()
    {
        return;
    }



    packet_receiver_fpa_builder::packet_receiver_fpa_builder() :
            _node_builder< packet_receiver_fpa >()
    {
    }

    packet_receiver_fpa_builder::~packet_receiver_fpa_builder()
    {
    }

    void packet_receiver_fpa_builder::apply_config( packet_receiver_fpa* a_node, const scarab::param_node& a_config )
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

} /* namespace psyllid */
