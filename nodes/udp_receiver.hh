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

namespace psyllid
{

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

    };

} /* namespace psyllid */

#endif /* NODES_UDP_RECEIVER_HH_ */
