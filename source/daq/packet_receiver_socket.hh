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

     @brief A producer to receive UDP packets via the standard socket interface and write them as raw blocks of memory

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "packet-receiver-socket"

     Available configuration values:
     - "length": uint -- The size of the output buffer
     - "max-packet-size": uint -- Maximum number of bytes to be read for each packet; larger packets will be truncated
     - "port": uint -- UDP port to listen on for packets
     - "ip": string -- IP port to listen on for packets; must be in IPV4 numbers-and-dots notation (e.g. 127.0.0.1)
     - "timeout-sec": uint -- Timeout (in seconds) while listening for incoming packets; listening for packets repeats after timeout

     Output Streams:
     - 0: memory_block
    */
    class packet_receiver_socket : public midge::_producer< packet_receiver_socket, typelist_1( memory_block ) >
    {
        public:
            packet_receiver_socket();
            virtual ~packet_receiver_socket();

        public:
            mv_accessible( uint64_t, length );
            mv_accessible( uint32_t, max_packet_size );
            mv_accessible( uint32_t, port );
            mv_referrable( std::string, ip );
            mv_accessible( unsigned, timeout_sec );  /// Timeout in seconds for waiting on socket recv function

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            void cleanup_socket();

            int f_socket;
            sockaddr_in* f_address;

        protected:
            int f_last_errno;

    };

    class packet_receiver_socket_binding : public _node_binding< packet_receiver_socket, packet_receiver_socket_binding >
    {
        public:
            packet_receiver_socket_binding();
            virtual ~packet_receiver_socket_binding();

        private:
            virtual void do_apply_config( packet_receiver_socket* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const packet_receiver_socket* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_RECEIVER_SOCKET_HH_ */
