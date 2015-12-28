/*
 * roach_packet.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef DATA_ROACH_PACKET_HH_
#define DATA_ROACH_PACKET_HH_

#include "macros.hh"
#include "types.hh"

namespace psyllid
{

    class roach_packet
    {
        public:
            roach_packet();
            virtual ~roach_packet();

            referrable( int8_t, time_value );
            referrable( midge::real_t, freq_value );
    };

} /* namespace psyllid */

#endif /* DATA_ROACH_PACKET_HH_ */
