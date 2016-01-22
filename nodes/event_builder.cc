/*
 * event_builder.cc
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#include "event_builder.hh"

#include "logger.hh"

using midge::stream;

namespace psyllid
{
    LOGGER( plog, "event_builder" );

    event_builder::event_builder() :
            f_length( 10 ),
            f_pretrigger( 0 ),
            f_skip_tolerance( 0 ),
            f_state( state_t::idle ),
            f_start_untriggered( 1 ),
            f_end_untriggered( 0 ),
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
        id_range_event* t_id_range = nullptr;

        while( true )
        {
            t_in_command = in_stream< 0 >().get();
            t_trigger_flag = in_stream< 0 >().data();
            t_id_range = out_stream< 0 >().data();

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
                if( f_state == state_t::idle )
                {
                    if( t_trigger_flag->get_flag() )
                    {
                        // start new event
                        f_state = state_t::triggered;
                        if( f_start_untriggered > f_end_untriggered /*f_untriggered_buffer.empty()*/ )
                        {
                            // there's no pretrigger available
                            t_id_range->set_start_id( t_current_id );
                        }
                        else
                        {
                            // add in the pretrigger
                            t_id_range->set_start_id( std::max( f_end_untriggered - f_pretrigger + 1, f_start_untriggered ) );
                        }
                        t_id_range->set_end_id( t_current_id );
                    }
                    else
                    {
                        // still untriggered
                        //f_untriggered_buffer.push_back( t_trigger_flag->get_id() );
                        // we can presume that f_start_untriggered is correct
                        f_end_untriggered = t_current_id;
                    }
                }
                else
                {
                    // currently triggered
                    if( t_trigger_flag->get_flag() )
                    {
                        // continue the current event
                        // we can assume the start_id is correct
                        t_id_range->set_end_id( t_current_id );

                        if( f_start_untriggered < f_end_untriggered )
                        {
                            // finished a skip span
                            f_end_untriggered = 0;
                            f_start_untriggered = 1;
                        }
                    }
                    else
                    {
                        // stop/skip
                        if( f_start_untriggered > f_end_untriggered )
                        {
                            // brand-new untriggered span
                            f_start_untriggered = t_current_id;
                        }
                        f_end_untriggered = t_current_id;

                        if( f_end_untriggered - f_start_untriggered + 1 > f_skip_tolerance )
                        {
                            DEBUG( plog, "Finished event: " << t_id_range->get_start_id() << " - " << t_id_range->get_end_id() );
                            // turn skip into event-stop
                            f_state = state_t::idle;
                            // the current event is done; advance the output stream
                            out_stream< 0 >().set( stream::s_run );
                        }

                    }
                }

                //DEBUG( plog, "Data " << t_trigger_flag->get_id() << " at " << (*t_freq_data->array())[0] << " resulted in flag <" << t_trigger_flag->get_flag() << ">" );

                //out_stream< 0 >().set( stream::s_run );
                continue;
            }

            if( t_in_command == stream::s_stop )
            {
                if( f_state == state_t::triggered )
                {
                    DEBUG( plog, "Finished event: " << t_id_range->get_start_id() << " - " << t_id_range->get_end_id() );
                    f_state = state_t::idle;
                    out_stream< 0 >().set( stream::s_run );
                }

                f_end_untriggered = 0;
                f_start_untriggered = 1;

                out_stream< 0 >().set( stream::s_stop );
                continue;
            }

            if( t_in_command == stream::s_exit )
            {
                if( f_state == state_t::triggered )
                {
                    DEBUG( plog, "Finished event: " << t_id_range->get_start_id() << " - " << t_id_range->get_end_id() );
                    f_state = state_t::idle;
                    out_stream< 0 >().set( stream::s_run );
                }

                f_end_untriggered = 0;
                f_start_untriggered = 1;

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
