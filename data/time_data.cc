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
            f_id( 0 ),
            f_array( new std::vector< int8_t >() )
    {
    }

    time_data::~time_data()
    {
    }

} /* namespace psyllid */
