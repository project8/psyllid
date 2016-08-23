/*
 * packet_distributor.cc
 *
 *  Created on: Aug 23, 2016
 *      Author: nsoblath
 */

#include "packet_distributor.hh"

namespace psyllid
{

    packet_distributor::packet_distributor() :
            f_read_it(),
            f_write_buffers()
    {
    }

    packet_distributor::~packet_distributor()
    {
    }

} /* namespace psyllid */
