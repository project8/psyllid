/*
 * tf_roach_monitor.cc
 *
 *  Created on: Sept 23, 2016
 *      Author: nsoblath
 */

#include "tf_roach_monitor.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "time.hh"

#include <limits>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( roach_time_monitor, "roach-time-monitor", roach_time_monitor_binding );
    REGISTER_NODE_AND_BUILDER( roach_freq_monitor, "roach-freq-monitor", roach_freq_monitor_binding );

    LOGGER( plog, "tf_roach_monitor" );


    //****************
    // Time Monitor
    //****************

    roach_time_monitor::roach_time_monitor() :
            f_last_pkt_in_batch( 0 ),
            f_packet_count( 0 ),
            f_acquisition_count( 0 )
    {
    }

    roach_time_monitor::~roach_time_monitor()
    {
    }

    void roach_time_monitor::initialize()
    {
        f_last_pkt_in_batch = std::numeric_limits< uint64_t >::max();
        return;
    }

    void roach_time_monitor::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_time_command = stream::s_none;

            time_data* t_time_data = nullptr;

            uint64_t t_current_pkt_in_batch = 0;

            while( ! is_canceled() )
            {
                t_time_command = in_stream< 0 >().get();
                if( t_time_command == stream::s_error ) break;
                if( t_time_command == stream::s_none ) continue;

                LTRACE( plog, "ROACH time monitor reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );

                if( t_time_command == stream::s_exit )
                {
                    LDEBUG( plog, "ROACH time monitor is exiting" );

                    break;
                }

                if( t_time_command == stream::s_stop )
                {
                    LDEBUG( plog, "ROACH time monitor is stopping" );

                    continue;
                }

                if( t_time_command == stream::s_start )
                {
                    LDEBUG( plog, "ROACH time monitor is starting" );

                    continue;
                }

                t_time_data = in_stream< 0 >().data();

                if( t_time_command == stream::s_run )
                {
                    ++f_packet_count;
                    t_current_pkt_in_batch = t_time_data->get_pkt_in_batch();

                    if( f_last_pkt_in_batch + 1 != t_current_pkt_in_batch )
                    {
                        ++f_acquisition_count;
                        LWARN( plog, "[Time] Packet-count discontinuity: last packet = " << f_last_pkt_in_batch << "  current packet = " << t_current_pkt_in_batch );
                    }
                    f_last_pkt_in_batch = t_current_pkt_in_batch;

                    continue;
                }

            }

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void roach_time_monitor::finalize()
    {
        LINFO( plog, "ROACH Time Monitor statistics:\n" <<
                "\tPacket count:\t" << f_packet_count << '\n' <<
                "\tAcq'n count:\t" << f_acquisition_count << '\n' <<
                "\tLast pkt in batch:\t" << f_last_pkt_in_batch );
        return;
    }


    roach_time_monitor_binding::roach_time_monitor_binding() :
            _node_binding< roach_time_monitor, roach_time_monitor_binding >()
    {
    }

    roach_time_monitor_binding::~roach_time_monitor_binding()
    {
    }

    void roach_time_monitor_binding::do_apply_config( roach_time_monitor*, const scarab::param_node& ) const
    {
        return;
    }

    void roach_time_monitor_binding::do_dump_config( const roach_time_monitor*, scarab::param_node& ) const
    {
        return;
    }



    //****************
    // Freq Monitor
    //****************

    roach_freq_monitor::roach_freq_monitor() :
            f_last_pkt_in_batch( 0 ),
            f_packet_count( 0 ),
            f_acquisition_count( 0 )
    {
    }

    roach_freq_monitor::~roach_freq_monitor()
    {
    }

    void roach_freq_monitor::initialize()
    {
        f_last_pkt_in_batch = std::numeric_limits< uint64_t >::max();
        return;
    }

    void roach_freq_monitor::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_freq_command = stream::s_none;

            freq_data* t_freq_data = nullptr;

            uint64_t t_current_pkt_in_batch = 0;

            while( ! is_canceled() )
            {
                t_freq_command = in_stream< 0 >().get();
                if( t_freq_command == stream::s_error ) break;
                if( t_freq_command == stream::s_none ) continue;

                LTRACE( plog, "ROACH freq monitor reading stream 0 (freq) at index " << in_stream< 0 >().get_current_index() );

                if( t_freq_command == stream::s_exit )
                {
                    LDEBUG( plog, "ROACH freq monitor is exiting" );

                    break;
                }

                if( t_freq_command == stream::s_stop )
                {
                    LDEBUG( plog, "ROACH freq monitor is stopping" );

                    continue;
                }

                if( t_freq_command == stream::s_start )
                {
                    LDEBUG( plog, "ROACH freq monitor is starting" );

                    continue;
                }

                t_freq_data = in_stream< 0 >().data();

                if( t_freq_command == stream::s_run )
                {
                    ++f_packet_count;
                    t_current_pkt_in_batch = t_freq_data->get_pkt_in_batch();

                    if( f_last_pkt_in_batch + 1 != t_current_pkt_in_batch )
                    {
                        ++f_acquisition_count;
                        LWARN( plog, "[Time] Packet-count discontinuity: last packet = " << f_last_pkt_in_batch << "  current packet = " << t_current_pkt_in_batch );
                    }
                    f_last_pkt_in_batch = t_current_pkt_in_batch;

                    continue;
                }

            }

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void roach_freq_monitor::finalize()
    {
        LINFO( plog, "ROACH Time Monitor statistics:\n" <<
                "\tPacket count:\t" << f_packet_count << '\n' <<
                "\tAcq'n count:\t" << f_acquisition_count << '\n' <<
                "\tLast pkt in batch:\t" << f_last_pkt_in_batch );
        return;
    }


    roach_freq_monitor_binding::roach_freq_monitor_binding() :
            _node_binding< roach_freq_monitor, roach_freq_monitor_binding >()
    {
    }

    roach_freq_monitor_binding::~roach_freq_monitor_binding()
    {
    }

    void roach_freq_monitor_binding::do_apply_config( roach_freq_monitor*, const scarab::param_node& ) const
    {
        return;
    }

    void roach_freq_monitor_binding::do_dump_config( const roach_freq_monitor*, scarab::param_node& ) const
    {
        return;
    }



} /* namespace psyllid */
