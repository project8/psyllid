/*
 * packet_receiver_socket.hh
 *
 *  Created on: Nov 2, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PACKET_RECEIVER_SOCKET_HH_
#define PSYLLID_PACKET_RECEIVER_SOCKET_HH_

#include "memory_block.hh"
#include "node_builder.hh"

#include "producer.hh"
#include "shared_cancel.hh"

#include <netinet/in.h>

namespace scarab
{
    class param_node;
}

namespace psyllid
{

    /*!
     @class packet_receiver_socket
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
    class packet_receiver_socket : public midge::_producer< packet_receiver_socket, typelist_1( memory_block ) >
    {
        public:
            packet_receiver_socket();
            virtual ~packet_receiver_socket();

        public:
            mv_accessible( uint64_t, length );
            mv_accessible( size_t, max_packet_size );
            mv_accessible( size_t, port );
            mv_referrable( std::string, ip );
            mv_accessible( unsigned, timeout_sec );  /// Timeout in seconds for waiting on socket recv function

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

        private:
            virtual void do_cancellation();

            int f_socket;
            sockaddr_in* f_address;

        protected:
            int f_last_errno;

    };

    class packet_receiver_socket_builder : public _node_builder< packet_receiver_socket >
    {
        public:
            packet_receiver_socket_builder();
            virtual ~packet_receiver_socket_builder();

        private:
            virtual void apply_config( packet_receiver_socket* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_RECEIVER_SOCKET_HH_ */
