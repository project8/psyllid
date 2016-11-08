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
    REGISTER_NODE_AND_BUILDER( roach_time_monitor, "roach-time-monitor" );

    LOGGER( plog, "tf_roach_monitor" );

    roach_time_monitor::roach_time_monitor() :
            f_last_pkt_in_batch( 0 )
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

            while( true )
            {
                t_time_command = in_stream< 0 >().get();
                LDEBUG( plog, "ROACH time monitor reading stream 0 (time) at index " << in_stream< 0 >().get_current_index() );

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
                    t_current_pkt_in_batch = t_time_data->get_pkt_in_batch();

                    if( f_last_pkt_in_batch + 1 != t_current_pkt_in_batch )
                    {
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
        return;
    }


    roach_time_monitor_builder::roach_time_monitor_builder() :
            _node_builder< roach_time_monitor >()
    {
    }

    roach_time_monitor_builder::~roach_time_monitor_builder()
    {
    }

    void roach_time_monitor_builder::apply_config( roach_time_monitor*, const scarab::param_node& )
    {
        return;
    }



} /* namespace psyllid */
