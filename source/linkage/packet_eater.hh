/*
 * packet_eater.hh
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PACKET_EATER_HH_
#define PSYLLID_PACKET_EATER_HH_

#include "packet_buffer.hh"

#include "cancelable.hh"
#include "member_variables.hh"

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
        receive_ring() : f_rd( nullptr ), f_map( nullptr ), f_req()
        {}
	};

    /*!
    @class packet_eater
    @author N.S. Oblath

    @brief Sucks up packets from a network connection and copies them to a circular buffer

    @details
    Input: mmap ring buffer
    Output: IP-packet circular buffer


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

	class packet_eater : public scarab::cancelable
	{
		public:
			packet_eater( const std::string& a_net_interface );
			virtual ~packet_eater();

			bool initialize();
			void attach_read_iterator( pb_iterator& a_iterator );

			void execute();

			packet_buffer& buffer();

			mv_accessible( unsigned, timeout_sec );  /// Timeout in seconds for waiting on the network interface
            mv_accessible( unsigned, n_blocks );     /// Number of blocks in the mmap ring buffer
			mv_accessible( unsigned, block_size );   /// Number of packets per block in the mmap ring buffer
			mv_accessible( unsigned, frame_size );   /// Number of blocks per frame in the mmap ring buffer

			mv_accessible( unsigned, buffer_size );  /// Number of packets in the IP-packet circular buffer

		private:
			void walk_block( block_desc* a_bd );
			void flush_block( block_desc* a_bd );

			bool process_packet( tpacket3_hdr* a_packet );

			std::string f_net_interface_name;
			int f_net_interface_index;

            int f_socket;

			receive_ring f_ring;

			uint64_t f_packets_total;
			uint64_t f_bytes_total;

			packet_buffer f_packet_buffer;
			pb_iterator f_iterator;

	};

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_EATER_HH_ */
