/*
 * udp_receiver.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_UDP_RECEIVER_HH_
#define PSYLLID_UDP_RECEIVER_HH_

#include "freq_data.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "producer.hh"
#include "shared_cancel.hh"

#include <memory>

namespace scarab
{
    class param_node;
}

namespace psyllid
{
    class udp_server;

    /*!
     @class udp_receiver
     @author N. S. Oblath

     @brief A UDP server to receive ROACH packets.

     @details


     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "udp-receiver"

     Available configuration values:
     - "time-length": uint -- The size of the output time-data buffer
     - "freq-length": uint -- The size of the output frequency-data buffer
     - "udp-buffer-size": uint -- The number of bytes in the UDP memory buffer for a single packet; generally this shouldn't be changed
     - "time-sync-tol": uint -- Tolerance for time synchronization between the ROACH and the server (seconds)
     - "server": node -- Options passed to the server

     Output Streams:
     - 0: time_data
     - 1: freq_data
    */
    class udp_receiver : public midge::_producer< udp_receiver, typelist_2( time_data, freq_data ) >
    {
        public:
            udp_receiver();
            virtual ~udp_receiver();

        public:
            mv_accessible( uint64_t, time_length );
            mv_accessible( uint64_t, freq_length );
            mv_accessible( size_t, udp_buffer_size );
            mv_accessible( unsigned, time_sync_tol );
            mv_assignable( scarab::param_node, server_config );

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

        private:
            bool f_paused;

            std::unique_ptr< udp_server > f_server;

            void id_match_sanity_check( uint64_t a_time_batch_pkt, uint64_t a_freq_batch_pkt, uint64_t a_time_session_pkt, uint64_t a_freq_session_pkt );

    };

    class udp_receiver_builder : public _node_builder< udp_receiver >
    {
        public:
            udp_receiver_builder();
            virtual ~udp_receiver_builder();

        private:
            virtual void apply_config( udp_receiver* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_UDP_RECEIVER_HH_ */
