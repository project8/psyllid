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


    REGISTER_PRESET( streaming_1ch, "str-1ch" );

    streaming_1ch::streaming_1ch()
    {
    	node( "udp-receiver", "udpr" );
    	node( "streaming-writer", "strw" );
    	node( "term_freq_data", "term" );

    	connection( "udpr.out_0:ew.in_0" );
    	connection( "udpr.out_1:term.in_0" );
    }

} /* namespace psyllid */
