/*
 * time_data.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#include "time_data.hh"

namespace psyllid
{

    time_data::time_data() :
            roach_packet_data(),
            f_pkt_in_session( 0 ),
            f_array( reinterpret_cast< iq_t* >( f_packet.f_data ) ),
            f_array_size( PAYLOAD_SIZE / 2 )
    {
    }

    time_data::~time_data()
    {
    }

} /* namespace psyllid */
