/*
 * roach_packet.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef DATA_ROACH_PACKET_HH_
#define DATA_ROACH_PACKET_HH_

#include "macros.hh"

namespace psyllid
{

    class roach_packet
    {
        public:
            roach_packet();
            virtual ~roach_packet();

            referrable( double, time_value );
            referrable( double, freq_value );
    };

} /* namespace psyllid */

#endif /* DATA_ROACH_PACKET_HH_ */
