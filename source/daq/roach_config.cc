/*
 * roach_config.cc
 *
 *  Created on: Feb 4, 2016
 *      Author: nsoblath
 */

#include "roach_config.hh"

namespace psyllid
{

    REGISTER_PRESET( streaming_1ch, "str-1ch" );

    streaming_1ch::streaming_1ch( const std::string& a_name ) :
            stream_preset( a_name )
    {
        node( "packet-receiver-socket", "prs" );
        node( "tf-roach-receiver", "tfrr" );
        node( "streaming-writer", "strw" );
        node( "term-freq-data", "term" );

        connection( "prs.out_0:tfrr.in_0" );
        connection( "tfrr.out_0:strw.in_0" );
        connection( "tfrr.out_1:term.in_0" );
    }

#ifdef __linux__
    REGISTER_PRESET( streaming_1ch_fpa, "str-1ch-fpa" );

    streaming_1ch_fpa::streaming_1ch_fpa( const std::string& a_name ) :
            stream_preset( a_name )
    {
        node( "packet-receiver-fpa", "prf" );
        node( "tf-roach-receiver", "tfrr" );
        node( "streaming-writer", "strw" );
        node( "term-freq-data", "term" );

        connection( "prf.out_0:tfrr.in_0" );
        connection( "tfrr.out_0:strw.in_0" );
        connection( "tfrr.out_1:term.in_0" );
    }
#endif

    REGISTER_PRESET( fmask_trigger_1ch,"fmask-1ch");

    fmask_trigger_1ch::fmask_trigger_1ch( const std::string& a_name ) :
            stream_preset( a_name )
    {
        node( "packet-receiver-socket", "prs" );
        node( "tf-roach-receiver", "tfrr");
        node( "frequency-mask-trigger", "fmt");
        //node( "term-time-data", "termtime" );
        //node( "term-trig-flag", "termtf" );
        node( "triggered-writer", "trw");

        connection( "prs.out_0:tfrr.in_0" );
        connection( "tfrr.out_0:trw.in_0" );
        connection( "tfrr.out_1:fmt.in_0" );
        //connection( "tfrr.out_0:termtime.in_0" );
        //connection( "fmt.out_0:termtf.in_0" );
        connection( "fmt.out_0:trw.in_1" );
    }

#ifdef __linux__
    REGISTER_PRESET( fmask_trigger_1ch_fpa,"fmask-1ch-fpa");
    fmask_trigger_1ch_fpa::fmask_trigger_1ch_fpa( const std::string& a_name ) :
            stream_preset( a_name )
    {
        node( "packet-receiver-fpa", "prf" );
        node( "tf-roach-receiver", "tfrr");
        node( "frequency-mask-trigger", "fmt");
        node( "triggered-writer", "trw");

        connection( "prf.out_0:tfrr.in_0" );
        connection( "tfrr.out_0:trw.in_0" );
        connection( "tfrr.out_1:fmt.in_0" );
        connection( "fmt.out_0:trw.in_1" );
    }
#endif

    REGISTER_PRESET( event_builder_1ch,"events-1ch");
    event_builder_1ch::event_builder_1ch( const std::string& a_name ) :
            stream_preset( a_name )
    {
        node( "packet-receiver-socket", "prs" );
        node( "tf-roach-receiver", "tfrr");
        node( "frequency-mask-trigger", "fmt");
        node( "event-builder", "eb");
        node( "triggered-writer", "trw");

        connection( "prs.out_0:tfrr.in_0" );
        connection( "tfrr.out_0:trw.in_0" );
        connection( "tfrr.out_1:fmt.in_0" );
        connection( "fmt.out_0:eb.in_0");
        connection( "eb.out_0:trw.in_1" );
    }

#ifdef __linux__
    REGISTER_PRESET( event_builder_1ch_fpa,"events-1ch-fpa");
    event_builder_1ch_fpa::event_builder_1ch_fpa( const std::string& a_name ) :
            stream_preset( a_name )
    {
        node( "packet-receiver-fpa", "prf" );
        node( "tf-roach-receiver", "tfrr");
        node( "frequency-mask-trigger", "fmt");
        node( "event-builder", "eb");
        node( "triggered-writer", "trw");

        connection( "prf.out_0:tfrr.in_0" );
        connection( "tfrr.out_0:trw.in_0" );
        connection( "tfrr.out_1:fmt.in_0" );
        connection( "fmt.out_0:eb.in_0");
        connection( "eb.out_0:trw.in_1" );
    }
#endif


} /* namespace psyllid */

