/*
 * packet_receiver_fpa.hh
 *
 *  Created on: Nov 2, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PACKET_RECEIVER_FPA_HH_
#define PSYLLID_PACKET_RECEIVER_FPA_HH_

#include "memory_block.hh"
#include "node_builder.hh"

#include "producer.hh"
#include "shared_cancel.hh"

#include <linux/if_packet.h>
#include <memory>
#include <sys/uio.h>

namespace scarab
{
    class param_node;
}

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
     @class packet_receiver_fpa
     @author N. S. Oblath

     @brief A producer to receive UDP packets via the fast-packet-acquisition interface and write them as raw blocks of memory

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Works in Linux only.

     Input: mmap ring buffer


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


     Node type: "packet-receiver-fpa"

     Available configuration values:
     - "length": uint -- The size of the output buffer
     - "max-packet-size": uint -- Maximum number of bytes to be read for each packet; larger packets will be truncated
     - "port": uint -- UDP port to listen on for packets
     - "interface": string -- Name of the network interface to listen on for packets
     - "timeout-sec": uint -- Timeout (in seconds) while listening for incoming packets; listening for packets repeats after timeout
     - "n-blocks": uint -- Number of blocks in the mmap ring buffer
     - "block-size": uint -- Number of packets per block in the mmap ring buffer
     - "frame-size": uint -- Number of blocks per frame in the mmap ring buffer

     Output Streams:
     - 0: memory_block
    */
    class packet_receiver_fpa : public midge::_producer< packet_receiver_fpa, typelist_1( memory_block ) >
    {
        public:
            packet_receiver_fpa();
            virtual ~packet_receiver_fpa();

        public:
            mv_accessible( uint64_t, length );
            mv_accessible( uint32_t, max_packet_size );
            mv_accessible( uint32_t, port );
            mv_referrable( std::string, interface );
            mv_accessible( unsigned, timeout_sec );  /// Timeout in seconds for waiting on the network interface
            mv_accessible( unsigned, n_blocks );     /// Number of blocks in the mmap ring buffer
            mv_accessible( unsigned, block_size );   /// Number of packets per block in the mmap ring buffer
            mv_accessible( unsigned, frame_size );   /// Number of blocks per frame in the mmap ring buffer

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            bool process_packet( tpacket3_hdr* a_packet );
            void cleanup_fpa();

            int f_net_interface_index;

            int f_socket;

            receive_ring f_ring;

            uint64_t f_packets_total;
            uint64_t f_bytes_total;
    };

    class packet_receiver_fpa_binding : public _node_binding< packet_receiver_fpa, packet_receiver_fpa_binding >
    {
        public:
            packet_receiver_fpa_binding();
            virtual ~packet_receiver_fpa_binding();

        private:
            virtual void do_apply_config( packet_receiver_fpa* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const packet_receiver_fpa* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_RECEIVER_FPA_HH_ */
