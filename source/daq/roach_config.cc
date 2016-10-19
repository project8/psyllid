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
        node( "tf-roach-receiver", "tfrr" );
    }


    REGISTER_PRESET( streaming_1ch, "str-1ch" );

    streaming_1ch::streaming_1ch()
    {
    	node( "tf-roach-receiver", "tfrr" );
    	node( "streaming-writer", "strw" );
    	node( "term-freq-data", "term" );

    	connection( "tfrr.out_0:strw.in_0" );
    	connection( "tfrr.out_1:term.in_0" );
    }

    REGISTER_PRESET( fmask_trigger_1ch,"fmask-1ch");

    fmask_trigger_1ch::fmask_trigger_1ch()
    {
	node( "tf-roach-receiver", "tfrr");
	node( "frequency-mask-trigger", "fmt");
	node( "egg-writer", "ew");

	connection( "tfrr.out_0:ew.in_0");
	connection( "tfrr.out_1:fmt.in_0");
	connection( "fmt.out_0:ew.in_1");
    }

} /* namespace psyllid */

