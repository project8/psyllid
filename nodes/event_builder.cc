/*
 * event_builder.cc
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#include "event_builder.hh"

#include "logger.hh"

#include <limits>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE( event_builder, "event-builder" );

    LOGGER( plog, "event_builder" );

    event_builder::event_builder() :
            f_length( 10 ),
            f_pretrigger( 0 ),
            f_skip_tolerance( 0 ),
            f_state( state_t::untriggered ),
            f_invalid_id( std::numeric_limits< uint64_t >::max() ),
            f_start_untriggered( f_invalid_id ),
            f_end_untriggered( f_invalid_id ),
            f_untriggered_buffer()
    {
    }

    event_builder::~event_builder()
    {
    }

    void event_builder::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void event_builder::execute()
    {
        midge::enum_t t_in_command = stream::s_none;
        trigger_flag* t_trigger_flag = nullptr;
        trigger_flag* t_write_flag = nullptr;

        while( true )
        {
            t_in_command = in_stream< 0 >().get();

            t_trigger_flag = in_stream< 0 >().data();
            t_write_flag = out_stream< 0 >().data();

            if( t_in_command == stream::s_start )
            {
                DEBUG( plog, "Starting the event builder" );
                out_stream< 0 >().set( stream::s_start );
                continue;
            }

            if( t_in_command == stream::s_run )
            {
                uint64_t t_current_id = t_trigger_flag->get_id();
                DEBUG( plog, "Event builder received id " << t_current_id );
                if( f_state == state_t::untriggered )
                {
                    // not triggered prior to the current packet

                    if( t_trigger_flag->get_flag() )
                    {
                        // start new event
                        DEBUG( plog, "Starting new event" );

                        f_state = state_t::triggered;
                        if( f_start_untriggered == f_invalid_id )
                        {
                            // there's no pretrigger available
                            advance_output_stream( t_write_flag, t_current_id, true );
                        }
                        else
                        {
                            // add in the pretrigger

                            // send write_flag == true for pre-trigger and current id
                            for( uint32_t t_id = std::max( f_end_untriggered - f_pretrigger + 1, f_start_untriggered );
                                    t_id <= t_current_id; ++t_id )
                            {
                                advance_output_stream( t_write_flag, std::max( f_end_untriggered - f_pretrigger + 1, f_start_untriggered ), true );
                            }
                            f_start_untriggered = f_invalid_id;
                            f_end_untriggered = f_invalid_id;
                        }
                    }
                    else
                    {
                        // still untriggered
                        DEBUG( plog, "Continuing to be untriggered" );

                        if( f_start_untriggered == f_invalid_id )
                        {
                            f_start_untriggered = t_current_id;
                        }
                        f_end_untriggered = t_current_id;

                        uint64_t t_intended_start_untriggered = f_end_untriggered - f_pretrigger + 1;
                        while( f_start_untriggered < t_intended_start_untriggered )
                        {
                            advance_output_stream( t_write_flag, f_start_untriggered, false );
                            ++f_start_untriggered;
                        }
                    }
                }
                else
                {
                    // already triggered prior to the current packet

                    if( t_trigger_flag->get_flag() )
                    {
                        // continue the current event
                        DEBUG( plog, "Continuing the event" );
                        advance_output_stream( t_write_flag, t_current_id, true );
                    }
                    else
                    {
                        // stop/skip
                        if( f_start_untriggered == f_invalid_id )
                        {
                            // brand-new untriggered span
                            f_start_untriggered = t_current_id;
                        }
                        f_end_untriggered = t_current_id;

                        if( f_end_untriggered - f_start_untriggered + 1 > f_skip_tolerance )
                        {
                            DEBUG( plog, "Finished event" );
                            // turn skip into event-stop
                            f_state = state_t::untriggered;

                            // in case the skip is larger than the pretrigger, we need to catch up handling the untriggered packets
                            uint64_t t_intended_start_untriggered = f_end_untriggered - f_pretrigger + 1;
                            while( f_start_untriggered < t_intended_start_untriggered )
                            {
                                advance_output_stream( t_write_flag, f_start_untriggered, false );
                                ++f_start_untriggered;
                            }
                        }

                    }
                }

                //DEBUG( plog, "Data " << t_trigger_flag->get_id() << " at " << (*t_freq_data->array())[0] << " resulted in flag <" << t_trigger_flag->get_flag() << ">" );

                //out_stream< 0 >().set( stream::s_run );
                continue;
            }

            if( t_in_command == stream::s_stop )
            {
                f_end_untriggered = f_invalid_id;
                f_start_untriggered = f_invalid_id;

                f_state = state_t::untriggered;

                out_stream< 0 >().set( stream::s_stop );
                continue;
            }

            if( t_in_command == stream::s_exit )
            {
                f_end_untriggered = f_invalid_id;
                f_start_untriggered = f_invalid_id;

                f_state = state_t::untriggered;

                out_stream< 0 >().set( stream::s_exit );
                break;
            }
        }
    }

    void event_builder::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }


} /* namespace psyllid */
