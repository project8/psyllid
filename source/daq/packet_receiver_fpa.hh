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

#include <memory>

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

     @brief A producer to receive and distribute time and frequency ROACH packets.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "tf-roach-receiver"

     Available configuration values:
     - "time-length": uint -- The size of the output time-data buffer
     - "freq-length": uint -- The size of the output frequency-data buffer
     - "udp-buffer-size": uint -- The number of bytes in the UDP memory buffer for a single packet; generally this shouldn't be changed
     - "time-sync-tol": uint -- Tolerance for time synchronization between the ROACH and the server (seconds)
     - "server": node -- Options passed to the server
       - "type": string -- Server type:
         - "socket" (default) = standard socket server (udp_server_socket)
         - "fpa" = fast-packet-acquisition (udp_server_fpa); requires executable run with root privileges
       - [specific-server dependent options]

     Output Streams:
     - 0: time_data
     - 1: freq_data
    */
    class packet_receiver_fpa : public midge::_producer< packet_receiver_fpa, typelist_1( memory_block ) >
    {
        public:
            packet_receiver_fpa();
            virtual ~packet_receiver_fpa();

        public:
            mv_accessible( uint64_t, length );
            mv_accessible( size_t, max_packet_size );
            mv_accessible( size_t, port );
            mv_referrable( std::string, interface );
            mv_accessible( unsigned, timeout_sec );  /// Timeout in seconds for waiting on the network interface
            mv_accessible( unsigned, n_blocks );     /// Number of blocks in the mmap ring buffer
            mv_accessible( unsigned, block_size );   /// Number of packets per block in the mmap ring buffer
            mv_accessible( unsigned, frame_size );   /// Number of blocks per frame in the mmap ring buffer

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

        private:
            virtual void do_cancellation();

            void walk_block( block_desc* a_bd );
            void flush_block( block_desc* a_bd );

            bool process_packet( tpacket3_hdr* a_packet );

            int f_net_interface_index;

            int f_socket;

            receive_ring f_ring;

            uint64_t f_packets_total;
            uint64_t f_bytes_total;
    };

    class packet_receiver_fpa_builder : public _node_builder< packet_receiver_fpa >
    {
        public:
            packet_receiver_fpa_builder();
            virtual ~packet_receiver_fpa_builder();

        private:
            virtual void apply_config( packet_receiver_fpa* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_RECEIVER_FPA_HH_ */
