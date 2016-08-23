/*
 * packet_eater.hh
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#ifndef SOURCE_PACKET_EATER_HH_
#define SOURCE_PACKET_EATER_HH_

#include "packet_buffer.hh"

#include "cancelable.hh"

#include <linux/if_packet.h>
#include <string>
#include <sys/uio.h>

namespace psyllid
{
	struct block_desc
	{
		uint32_t f_version;
		uint32_t f_offset_to_priv;
		tpacket_hdr_v1 f_packet_hdr;
	};

	struct receive_ring
	{
		iovec* f_rd;
		uint8_t* f_map;
		tpacket_req3 f_req;
	};

	/*!
    @class packet_eater
    @author N.S. Oblath

    @brief Sucks up packets from a network connection and distributes them to a specified number of circular buffers

    @details
    Following documentation of using mmap ring buffers in networking here:
        https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt

    Understanding of tpacket3_hdr that I've pieced together:

        Defined in if_packet.h (my comments denoted with ///)

        struct tpacket_hdr_variant1 {
            __u32   tp_rxhash;           ///
            __u32   tp_vlan_tci;         ///
        };

        struct tpacket3_hdr {
            __u32       tp_next_offset;  /// Memory offset from the packet location to the next packet
            __u32       tp_sec;          ///
            __u32       tp_nsec;         ///
            __u32       tp_snaplen;      /// Size of this packet in bytes
            __u32       tp_len;          ///
            __u32       tp_status;       /// Packet status (see values below)
            __u16       tp_mac;          /// Memory offset from the packet location to the ethhdr object
            __u16       tp_net;          ///
            // pkt_hdr variants
            union {
                struct tpacket_hdr_variant1 hv1;
            };
        };

        Available values for tp_status from if_packet.h:

        // Rx ring - header status
        #define TP_STATUS_KERNEL    0x0     /// Returned to the kernel?
        #define TP_STATUS_USER      0x1     /// Available to the user?
        #define TP_STATUS_COPY      0x2
        #define TP_STATUS_LOSING    0x4
        #define TP_STATUS_CSUMNOTREADY  0x8
        #define TP_STATUS_VLAN_VALID   0x10 // auxdata has valid tp_vlan_tci
        #define TP_STATUS_BLK_TMO   0x20


	 */

	class packet_eater : scarab::cancelable
	{
		public:
			packet_eater( const std::string& a_net_interface, unsigned a_timeout_sec = 1, unsigned a_n_blocks = 64, unsigned a_block_size = 1 << 22, unsigned a_frame_size = 1 << 11 );
			virtual ~packet_eater();

			void execute();

		private:
			void walk_block( block_desc* a_bd );
			void flush_block( block_desc* a_bd );

			void process_packet( tpacket3_hdr* a_packet );

			unsigned f_timeout_sec;

            int f_socket;

			receive_ring* f_ring;
	        unsigned f_block_size;
	        unsigned f_frame_size;
	        unsigned f_n_blocks;


			sockaddr_ll* f_address;

			uint64_t f_packets_total;
			uint64_t f_bytes_total;

			packet_buffer f_packet_buffer;
			pb_iterator f_iterator;

	};

} /* namespace psyllid */

#endif /* SOURCE_PACKET_EATER_HH_ */
