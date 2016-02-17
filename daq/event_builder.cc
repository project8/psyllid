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
            f_state( state_t::filling_pretrigger ),
            f_pretrigger_buffer()
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
        f_pretrigger_buffer.clear();
        if( f_pretrigger == 0 ) f_state = state_t::untriggered_nopt;
        else f_state = state_t::filling_pretrigger;

        midge::enum_t t_in_command = stream::s_none;
        trigger_flag* t_trigger_flag = nullptr;
        trigger_flag* t_write_flag = nullptr;

        bool t_current_trig_flag = false;
        uint64_t t_current_id = 0;

        while( true )
        {
            t_in_command = in_stream< 0 >().get();
            DEBUG( plog, "Event builder reading stream at index " << in_stream< 0 >().get_current_index() );

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
                t_current_trig_flag = t_trigger_flag->get_flag();
                t_current_id = t_trigger_flag->get_id();
                DEBUG( plog, "Event builder received id <" << t_current_id << "> with flag value <" << t_trigger_flag->get_flag() << ">" );

                if( f_state == state_t::filling_pretrigger )
                {
                    if( t_current_trig_flag )
                    {
                        DEBUG( plog, "Was: filling pt; Now: new trigger" );

                        while( ! f_pretrigger_buffer.empty() )
                        {
                            t_write_flag->set_id( f_pretrigger_buffer.front() );
                            t_write_flag->set_flag( true );
                            DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                            out_stream< 0 >().set( midge::stream::s_run );
                            f_pretrigger_buffer.pop_front();
                        }

                        t_write_flag->set_id( t_trigger_flag->get_id() );
                        t_write_flag->set_flag( true );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        f_state = state_t::new_trigger;
                        continue;
                    }
                    else
                    {
                        f_pretrigger_buffer.push_back( t_trigger_flag->get_id() );
                        if( f_pretrigger_buffer.size() == f_pretrigger )
                        {
                            DEBUG( plog, "Was: filling pt; Now: untriggered" );

                            f_state = state_t::untriggered;
                            continue;
                        }
                        else
                        {
                            DEBUG( plog, "Was: filling pt; Now: filling pt" );
                            continue;
                        }
                    }
                }
                if( f_state == state_t::untriggered )
                {
                    if( t_current_trig_flag )
                    {
                        DEBUG( plog, "Was: untriggered; Now: new trigger" );

                        while( ! f_pretrigger_buffer.empty() )
                        {
                            t_write_flag->set_id( f_pretrigger_buffer.front() );
                            t_write_flag->set_flag( true );
                            DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                            out_stream< 0 >().set( midge::stream::s_run );
                            f_pretrigger_buffer.pop_front();
                        }

                        t_write_flag->set_id( t_trigger_flag->get_id() );
                        t_write_flag->set_flag( true );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        f_state = state_t::new_trigger;
                        continue;
                    }
                    else
                    {
                        DEBUG( plog, "Was: untriggered; Now: untriggered" );

                        f_pretrigger_buffer.push_back( t_trigger_flag->get_id() );

                        t_write_flag->set_id( f_pretrigger_buffer.front() );
                        t_write_flag->set_flag( false );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        f_pretrigger_buffer.pop_front();

                        continue;
                    }
                }
                if( f_state == state_t::untriggered_nopt )
                {
                    if( t_current_trig_flag )
                    {
                        DEBUG( plog, "Was: untriggered - no pt; Now: new trigger" );

                        t_write_flag->set_id( t_trigger_flag->get_id() );
                        t_write_flag->set_flag( true );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        f_state = state_t::new_trigger;
                        continue;
                    }
                    else
                    {
                        DEBUG( plog, "Was: untriggered - no pt; Now: untriggered - no pt" );

                        t_write_flag->set_id( t_trigger_flag->get_id() );
                        t_write_flag->set_flag( false );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        continue;
                    }
                }
                else if( f_state == state_t::new_trigger )
                {
                    if( t_current_trig_flag )
                    {
                        DEBUG( plog, "Was: new trigger; Now: triggered" );

                        t_write_flag->set_id( t_trigger_flag->get_id() );
                        t_write_flag->set_flag( true );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        f_state = state_t::triggered;
                        continue;
                    }
                    else
                    {
                        if( f_pretrigger == 0 )
                        {
                            DEBUG( plog, "Was: new trigger; Now: untriggered - no pt" );

                            t_write_flag->set_id( t_trigger_flag->get_id() );
                            t_write_flag->set_flag( false );
                            DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                            out_stream< 0 >().set( midge::stream::s_run );

                            f_state = state_t::untriggered_nopt;
                            continue;
                        }
                        else
                        {
                            f_pretrigger_buffer.push_back( t_trigger_flag->get_id() );
                            if( f_pretrigger > 1 )
                            {
                                DEBUG( plog, "Was: untriggered; Now: filling pt" );

                                f_state = state_t::filling_pretrigger;
                                continue;
                            }
                            else
                            {
                                DEBUG( plog, "Was: new trigger; Now: untriggered" );

                                f_state = state_t::untriggered;
                                continue;
                            }
                        }
                    }
                }
                else if( f_state == state_t::triggered )
                {
                    if( t_current_trig_flag )
                    {
                        DEBUG( plog, "Was: triggered; Now: triggered" );

                        t_write_flag->set_id( t_trigger_flag->get_id() );
                        t_write_flag->set_flag( true );
                        DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                        out_stream< 0 >().set( midge::stream::s_run );

                        continue;
                    }
                    else
                    {
                        if( f_pretrigger == 0 )
                        {
                            DEBUG( plog, "Was: triggered; Now: untriggered - no pt" );

                            t_write_flag->set_id( t_trigger_flag->get_id() );
                            t_write_flag->set_flag( false );
                            DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                            out_stream< 0 >().set( midge::stream::s_run );

                            f_state = state_t::untriggered_nopt;
                            continue;
                        }
                        else
                        {
                            f_pretrigger_buffer.push_back( t_trigger_flag->get_id() );
                            if( f_pretrigger > 1 )
                            {
                                DEBUG( plog, "Was: triggered; Now: filling pt" );

                                f_state = state_t::filling_pretrigger;
                                continue;
                            }
                            else
                            {
                                DEBUG( plog, "Was: triggered; Now: untriggered" );

                                f_state = state_t::untriggered;
                                continue;
                            }
                        }
                    }
                }

            } // end if( t_in_command == stream::s_run )


            if( t_in_command == stream::s_stop )
            {
                DEBUG( plog, "Event builder is stopping at stream index " << out_stream< 0 >().get_current_index() );

                while( ! f_pretrigger_buffer.empty() )
                {
                    t_write_flag->set_id( f_pretrigger_buffer.front() );
                    t_write_flag->set_flag( true );
                    DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                    out_stream< 0 >().set( stream::s_run );
                    f_pretrigger_buffer.pop_front();
                }
                f_state = state_t::filling_pretrigger;

                out_stream< 0 >().set( stream::s_stop );
                continue;
            }

            if( t_in_command == stream::s_exit )
            {
                DEBUG( plog, "Event builder is exiting at stream index " << out_stream< 0 >().get_current_index() );

                while( ! f_pretrigger_buffer.empty() )
                {
                    t_write_flag->set_id( f_pretrigger_buffer.front() );
                    t_write_flag->set_flag( true );
                    DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
                    out_stream< 0 >().set( stream::s_run );
                    f_pretrigger_buffer.pop_front();
                }
                f_state = state_t::filling_pretrigger;

                out_stream< 0 >().set( stream::s_exit );
                break;
            }

        } // end while( true )
    }

    void event_builder::advance_output_stream( trigger_flag* a_write_flag, uint64_t a_id, bool a_trig_flag )
    {
         a_write_flag->set_id( a_id );
         a_write_flag->set_flag( a_trig_flag );
         DEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
         out_stream< 0 >().set( midge::stream::s_run );
         return;
    }

    void event_builder::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }

} /* namespace psyllid */
