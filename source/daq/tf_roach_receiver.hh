/*
 * udp_receiver.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_TF_ROACH_RECEIVER_HH_
#define PSYLLID_TF_ROACH_RECEIVER_HH_

#include "freq_data.hh"
#include "memory_block.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "transformer.hh"
#include "shared_cancel.hh"

#include <memory>

namespace scarab
{
    class param_node;
}

namespace psyllid
{
    /*!
     @class tf_roach_receiver
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
    class tf_roach_receiver : public midge::_transformer< tf_roach_receiver, typelist_1( memory_block ), typelist_2( time_data, freq_data ) >
    {
        public:
            tf_roach_receiver();
            virtual ~tf_roach_receiver();

        public:
            mv_accessible( uint64_t, time_length );
            mv_accessible( uint64_t, freq_length );
            mv_accessible( size_t, udp_buffer_size );
            mv_accessible( unsigned, time_sync_tol );

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

        private:
            void check_instruction();
            virtual void do_cancellation();

            bool f_paused;

            void id_match_sanity_check( uint64_t a_time_batch_pkt, uint64_t a_freq_batch_pkt, uint64_t a_time_session_pkt, uint64_t a_freq_session_pkt );

    };

    class tf_roach_receiver_builder : public _node_builder< tf_roach_receiver >
    {
        public:
            tf_roach_receiver_builder();
            virtual ~tf_roach_receiver_builder();

        private:
            virtual void apply_config( tf_roach_receiver* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_TF_ROACH_RECEIVER_HH_ */
