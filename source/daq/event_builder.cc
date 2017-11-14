/*
 * event_builder.cc
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#include "event_builder.hh"

#include <limits>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( event_builder, "event-builder", event_builder_binding );

    LOGGER( plog, "event_builder" );

    event_builder::event_builder() :
            f_length( 10 ),
            f_pretrigger( 0 ),
            f_skip_tolerance( 0 ),
            f_state( state_t::untriggered ),
            f_pretrigger_buffer(),
            f_skip_buffer()
    {
    }

    event_builder::~event_builder()
    {
    }

    void event_builder::initialize()
    {
        f_pretrigger_buffer.resize( f_pretrigger + 1 );
        f_skip_buffer.resize( f_skip_tolerance + 1);
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void event_builder::execute( midge::diptera* a_midge )
    {
        try
        {
            f_pretrigger_buffer.clear();
            f_skip_buffer.clear();
            f_state = state_t::untriggered;

            midge::enum_t t_in_command = stream::s_none;
            trigger_flag* t_trigger_flag = nullptr;
            trigger_flag* t_write_flag = nullptr;

            bool t_current_trig_flag = false;

            while( ! is_canceled() )
            {
                t_in_command = in_stream< 0 >().get();
                if( t_in_command == stream::s_none ) continue;
                if( t_in_command == stream::s_error ) break;

                LTRACE( plog, "Event builder reading stream at index " << in_stream< 0 >().get_current_index() );

                t_trigger_flag = in_stream< 0 >().data();
                t_write_flag = out_stream< 0 >().data();

                if( t_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting the event builder" );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    continue;
                }

                if( t_in_command == stream::s_run )
                {
                    t_current_trig_flag = t_trigger_flag->get_flag();

                    LDEBUG( plog, "Event builder received id <" << t_trigger_flag->get_id() << "> with flag value <" << t_trigger_flag->get_flag() << ">" );

                    // if currently untriggered, fill pretrigger buffer
                    if (f_state == state_t::untriggered)
                    {
                        f_pretrigger_buffer.push_back(t_trigger_flag->get_id());
                    }
                    /*// if state is triggered fill skip buffer
                    else if (f_state == state_t::triggered)
                    {
                        // put the new data in the pretrigger buffer
                        f_skip_buffer.push_back( t_trigger_flag->get_id() );
                    }*/
                    // if state is skipping or triggered fill both buffers
                    else
                    {
                        f_skip_buffer.push_back( t_trigger_flag->get_id());
                        f_pretrigger_buffer.push_back( t_trigger_flag->get_id());
                    }

                    if( f_state == state_t::untriggered )
                    {
                        LDEBUG( plog, "Currently in untriggered state" );
                        if( t_current_trig_flag )
                        {
                            LINFO( plog, "New trigger; flushing pretrigger buffer" );
                            // flush the pretrigger buffer as true, which includes the current trig id
                            while( ! f_pretrigger_buffer.empty() )
                            {
                                LDEBUG( plog, "Pretrigger id <" << f_pretrigger_buffer.front() );
                                if( ! write_output_from_ptbuff_front( true, t_write_flag ) )
                                {
                                    goto exit_outer_loop;
                                }
                                // advance our output data pointer to the next in the stream
                                t_write_flag = out_stream< 0 >().data();
                            }

                            // set state to triggered
                            f_state = state_t::triggered;

                            // pretrigger buffer should be empty
                        }
                        else
                        {
                            LTRACE( plog, "No new trigger; Writing to from pretrig buffer only if buffer is full: " << f_pretrigger_buffer.full() );
                            // contents of the buffer are the existing pretrigger plus the current trig id
                            // only write out from the front of the buffer if the buffer is full; otherwise we're filling the buffer
                            if( f_pretrigger_buffer.full() )
                            {
                                if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                                {
                                    break;
                                }
                                // pretrigger buffer is full - 1
                            }
                        }
                    }
                    else if( f_state == state_t::triggered )
                    {
                        LDEBUG( plog, "Currently in triggered state" );
                        if( t_current_trig_flag )
                        {
                            LDEBUG( plog, "Continuing as triggered" );
                            // contents of the buffer (the current trig id) need to be written out
                            // write the one thing in the pt buffer as true, which is the current trig id
                            if( ! write_output_from_skipbuff_front( true, t_write_flag ) )
                            {
                                break;
                            }
                            // current front of pretrigger has already been written
                            f_pretrigger_buffer.pop_front();
                        }
                        else
                        {
                            LDEBUG( plog, "No new trigger; Switching state" );
                            // contents of the skip buffer (the current trig id) are the first ids to be skipped
                            // only write out if the buffer is full (in this case, equivalent to f_skip_tolerance == 0)
                            if( f_skip_buffer.full() )
                            {
                                // no need to write, id is also stored in pretrigger buffer
                                f_skip_buffer.clear();
                                LDEBUG( plog, "Next state is untriggered");
                                // in this case, next state is untriggered
                                f_state = state_t::untriggered;
                            }
                            else
                            {
                                LDEBUG( plog, "Next state is skipping");
                                // set state to untriggered
                                f_state = state_t::skipping;
                            }
                            // if pretrigger is 0
                            if (f_pretrigger_buffer.full())
                            {
                                // write the one thing in the pt buffer as false, which is the current trig id
                                if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                                {
                                    break;
                                }
                            }
                        }
                    }
                    else if( f_state == state_t::skipping)
                    {
                        LDEBUG( plog, "Currently in skipping state" );

                        if( t_current_trig_flag )
                        {
                            LTRACE( plog, "New trigger; flushing skip buffer" );
                            while( ! f_skip_buffer.empty() )
                            {
                                LDEBUG( plog, "true skip id <" << f_skip_buffer.front() );
                                if( ! write_output_from_skipbuff_front( true, t_write_flag ) )
                                {
                                    goto exit_outer_loop;
                                }
                                // advance our output data pointer to the next in the stream
                                t_write_flag = out_stream< 0 >().data();
                            }
                            // also remove all entries from pretrigger buffer
                            // ids were already written
                            f_pretrigger_buffer.clear();

                            // set state to triggered
                            f_state = state_t::triggered;
                        }
                        else
                        {
                            if(f_skip_buffer.full())
                            {
                                LDEBUG( plog, "Skip_tolerance reached. Continuing as untriggered");
                                // write out ids as false that are only in the skip buffer
                                while( f_skip_buffer.size() > f_pretrigger_buffer.size() )
                                {
                                    LDEBUG( plog, "false skip id <" << f_skip_buffer.front() );
                                    if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                                    {
                                        goto exit_outer_loop;
                                    }
                                    // advance our output data pointer to the next in the stream
                                    t_write_flag = out_stream< 0 >().data();
                                }
                                // then delete the remaining content of the skip buffer
                                f_skip_buffer.clear();

                                // contents of the buffer are the existing pretrigger plus the current trig id
                                // only write out from the front of the buffer if the buffer is full; otherwise we're filling the buffer
                                if( f_pretrigger_buffer.full() )
                                {
                                    if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                                    {
                                        break;
                                    }
                                }
                                // set state to untriggered
                                f_state = state_t::untriggered;
                            }

                            else
                            {
                                // if pretrigger is full remove first item, no need to write, ids are also in skip buffer and will be written from there
                                if(f_pretrigger_buffer.full())
                                {
                                    f_pretrigger_buffer.pop_front();
                                }
                                LTRACE(plog, "No new trigger. Continue to fill skip and pretrigger buffer buffer.")
                            }
                        }
                    }
                } // end if( t_in_command == stream::s_run )


                if( t_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "Event builder is stopping at stream index " << out_stream< 0 >().get_current_index() );

                    LDEBUG( plog, "Flushing pretrigger buffer as untriggered" );
                    while( ! f_pretrigger_buffer.empty() )
                    {
                        LTRACE( plog, "Pretrigger id <" << f_pretrigger_buffer.front() );
                        if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                        {
                            goto exit_outer_loop;
                        }
                        // advance our output data pointer to the next in the stream
                        t_write_flag = out_stream< 0 >().data();
                    }

                    LDEBUG( plog, "Flushing skip buffer as untriggered" );
                    while( ! f_skip_buffer.empty() )
                    {
                        LTRACE( plog, "Skip id <" << f_skip_buffer.front() );
                        if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                        {
                            goto exit_outer_loop;
                        }
                        // advance our output data pointer to the next in the stream
                        t_write_flag = out_stream< 0 >().data();
                    }

                    f_state = state_t::untriggered;

                    if( ! out_stream< 0 >().set( stream::s_stop ) )
                    {
                        LERROR( plog, "Exiting due to stream error" );
                        break;
                    }
                    continue;
                }

                if( t_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "Event builder is exiting at stream index " << out_stream< 0 >().get_current_index() );

                    LDEBUG( plog, "Flushing pretrigger buffer as untriggered" );
                    while( ! f_pretrigger_buffer.empty() )
                    {
                        LTRACE( plog, "Pretrigger id <" << f_pretrigger_buffer.front() );
                        if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                        {
                            goto exit_outer_loop;
                        }
                        // advance our output data pointer to the next in the stream
                        t_write_flag = out_stream< 0 >().data();
                    }

                    LDEBUG( plog, "Flushing skip buffer as untriggered" );
                    while( ! f_skip_buffer.empty() )
                    {
                        LTRACE( plog, "Skip id <" << f_skip_buffer.front() );
                        if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                        {
                            goto exit_outer_loop;
                        }
                        // advance our output data pointer to the next in the stream
                        t_write_flag = out_stream< 0 >().data();
                    }
                    f_state = state_t::untriggered;

                    out_stream< 0 >().set( stream::s_exit );
                    break;
                }

            } // end while( ! is_canceled() )

exit_outer_loop:
            LDEBUG( plog, "Stopping output stream" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void event_builder::advance_output_stream( trigger_flag* a_write_flag, uint64_t a_id, bool a_trig_flag )
    {
         a_write_flag->set_id( a_id );
         a_write_flag->set_flag( a_trig_flag );
         LDEBUG( plog, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
         out_stream< 0 >().set( midge::stream::s_run );
         return;
    }

    void event_builder::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }


    event_builder_binding::event_builder_binding() :
            _node_binding< event_builder, event_builder_binding >()
    {
    }

    event_builder_binding::~event_builder_binding()
    {
    }

    void event_builder_binding::do_apply_config( event_builder* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring event_builder with:\n" << a_config );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        a_node->set_pretrigger( a_config.get_value( "pretrigger", a_node->get_pretrigger() ) );
        a_node->set_skip_tolerance( a_config.get_value( "skip-tolerance", a_node->get_skip_tolerance() ) );
        return;
    }

    void event_builder_binding::do_dump_config( const event_builder* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for event_builder" );
        a_config.add( "length", new scarab::param_value( a_node->get_length() ) );
        a_config.add( "pretrigger", new scarab::param_value( a_node->get_pretrigger() ) );
        a_config.add( "skip-tolerance", new scarab::param_value( a_node->get_skip_tolerance() ) );
        return;
    }


} /* namespace psyllid */
