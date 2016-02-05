/*
 * roach_config.cc
 *
 *  Created on: Feb 4, 2016
 *      Author: nsoblath
 */

#include "roach_config.hh"

namespace psyllid
{

    REGISTER_PRESET( roach_config, "roach" );

    roach_config::roach_config()
    {
        node( "udp-receiver", "udpr" );
    }

    roach_config::~roach_config()
    {
    }

} /* namespace psyllid */
