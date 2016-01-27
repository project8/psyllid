/*
 * freq_data.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#include "freq_data.hh"

namespace psyllid
{

    freq_data::freq_data() :
            roach_packet_data(),
            f_array( reinterpret_cast< const iq_t* >( f_packet.f_data ) ),
            f_array_size( PAYLOAD_SIZE / 2 )
    {
    }

    freq_data::~freq_data()
    {
    }

} /* namespace psyllid */
