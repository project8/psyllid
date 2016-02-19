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

namespace psyllid
{

    /*!
     @class udp-receiver
     @author N. S. Oblath

     @brief A UDP server to receive ROACH packets.

     @details


     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "udp-receiver"

     Available configuration values:
     - "time-length": uint -- The size of the output time-data buffer
     - "freq-length": uint -- The size of the output frequency-data buffer
     - "port": uint -- The port number to listen on for packets
     - "udp-buffer-size": uint -- The number of bytes in the UDP memory buffer for a single packet; generally this shouldn't be changed

     Output Streams:
     - 0: time_data
     - 1: freq_data
    */
    class udp_receiver :
            public midge::_producer< udp_receiver, typelist_2( time_data, freq_data ) >
    {
        public:
            udp_receiver();
            virtual ~udp_receiver();

        public:
            accessible( uint64_t, time_length );
            accessible( uint64_t, freq_length );
            accessible( uint64_t, port );
            accessible( size_t, udp_buffer_size );

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

        private:
            bool f_paused;

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
