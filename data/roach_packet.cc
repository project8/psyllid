/*
 * roach_packet.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "roach_packet.hh"

namespace psyllid
{

    roach_packet::roach_packet() :
            f_time_value( 0. ),
            f_freq_value( 0. )
    {
    }

    roach_packet::~roach_packet()
    {
    }

} /* namespace psyllid */
