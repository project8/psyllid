/*
 * udp_receiver.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_UDP_RECEIVER_HH_
#define PSYLLID_UDP_RECEIVER_HH_

#include "roach_packet.hh"

#include "macros.hh"
#include "producer.hh"

using namespace midge;

namespace psyllid
{

    class udp_receiver :
            public _producer< udp_receiver, typelist_1( roach_packet ) >
    {
        public:
            udp_receiver();
            virtual ~udp_receiver();

        public:
            accessible( count_t, length );

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

    };

} /* namespace psyllid */

#endif /* NODES_UDP_RECEIVER_HH_ */
