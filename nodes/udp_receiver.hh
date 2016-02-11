/*
 * udp_receiver.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef NODES_UDP_RECEIVER_HH_
#define NODES_UDP_RECEIVER_HH_

#include "freq_data.hh"
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

} /* namespace psyllid */

#endif /* NODES_UDP_RECEIVER_HH_ */
