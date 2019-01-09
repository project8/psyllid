/*
 * global_config.cc
 *
 *  Created on: Aug 25, 2016
 *      Author: nsoblath
 */

#include "global_config.hh"

namespace psyllid
{

    void global_config::set_config( const scarab::param_node& a_config )
    {
        f_config = a_config;
    }

    const scarab::param_node& global_config::retrieve() const
    {
        return f_config;
    }

    global_config::global_config() :
            singleton< global_config >(),
            f_config()
    {
    }

    global_config::~global_config()
    {
    }

} /* namespace scarab */

