/*
 * roach_packet.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef DATA_ROACH_PACKET_HH_
#define DATA_ROACH_PACKET_HH_

#include "member_variables.hh"

#include <cinttypes>

namespace psyllid
{

    class roach_packet
    {
        public:
            roach_packet();
            virtual ~roach_packet();

            mv_referrable( int8_t, time_value );
            mv_referrable( double, freq_value );
    };

} /* namespace psyllid */

#endif /* DATA_ROACH_PACKET_HH_ */
