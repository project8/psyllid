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
            f_pre_trigger_buffer_size( 0 ),
            f_event_buffer_size( 0 ),
            f_post_trigger_buffer_size( 0 ),
            f_n_triggers( 1 ),
            f_state( state_t::untriggered ),
            f_pre_trigger_buffer(),
            f_event_buffer(),
            f_post_trigger_buffer()
    {
    }

    event_builder::~event_builder()
    {
    }

    void event_builder::initialize()
    {
        f_pre_trigger_buffer.resize( f_pre_trigger_buffer_size + 1 );
        f_event_buffer.resize( f_event_buffer_size + 1);
        f_post_trigger_buffer.resize( f_post_trigger_buffer_size + 1 );
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void event_builder::execute( midge::diptera* a_midge )
    {
        try
        {
            f_pre_trigger_buffer.clear();
            f_event_buffer.clear();
            f_post_trigger_buffer.clear();
            f_state = state_t::untriggered;

            // 1 in_command for every state
            midge::enum_t t_in_command = stream::s_none;

            trigger_flag* t_minor_trigger_flag = nullptr;
            trigger_flag* t_major_trigger_flag = nullptr;
            trigger_flag* t_post_trigger_flag = nullptr;
            trigger_flag* t_write_flag = nullptr;

            bool t_current_trig_flag = false;
            bool t_skip_next_state_flag = false;

            while( ! is_canceled() )
            {
                t_in_command = in_stream< 0 >().get();

                if( t_in_command == stream::s_none ) continue;
                if( t_in_command == stream::s_error ) break;

                LTRACE( plog, "Event builder reading stream at index " << in_stream< 0 >().get_current_index() );

                t_minor_trigger_flag = in_stream< 0 >().data();
                t_major_trigger_flag = in_stream< 1 >().data();
                t_post_trigger_flag = in_stream< 2 >().data();
                t_write_flag = out_stream< 3 >().data();

                if( t_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting the event builder" );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    continue;
                }

                if( t_in_command == stream::s_run )
                {

                    LTRACE( plog, "Event builder received id <" << t_minor_trigger_flag->get_id() << "> with flag value <" << t_trigger_flag->get_flag() << ">" );

                    // fill buffers and set flags depending on state
                    if( f_state == state_t::untriggered )
                    {
                        t_current_trig_flag = t_minor_trigger_flag->get_flag();
                        t_skip_next_state_flag = t_major_trigger_flag->get_flag();

                        f_pre_trigger_buffer.push_back(t_minor_trigger_flag->get_id());
                    }
                    else if( f_state == state_t::possibly_triggered )
                    {
                        t_current_trig_flag = t_major_trigger_flag->get_flag();
                        t_skip_next_state_flag = false;

                        f_event_buffer.push_back( t_major_trigger_flag->get_id());
                    }
                    else
                    {
                        t_current_trig_flag = t_post_trigger_flag->get_flag();
                        t_skip_next_state_flag = false;

                        f_post_trigger_buffer.push_back( t_post_trigger_flag->get_id());
                    }

                    // states
                    if( f_state == state_t::untriggered )
                    {
                        LTRACE( plog, "Untriggered state" );
                        if( t_skip_next_state_flag == true)
                        {
                            LINFO( plog, "New trigger. Going to skip next state" );
                            LINFO( plog, "Next state is triggered" );
                            f_state = state_t::triggered;
                        }
                        else if( t_current_trig_flag == true )
                        {
                            // set state to possibly triggered
                            LDEBUG( plog, "Next state is possibly_triggered" );
                            f_state = state_t::possibly_triggered;
                        }
                        else
                        {
                            LTRACE( plog, "No new trigger; Writing to from pretrig buffer only if buffer is full: " << f_pre_trigger_buffer.full() );
                            // contents of the buffer are the existing pretrigger plus the current trig id
                            // only write out from the front of the buffer if the buffer is full; otherwise we're filling the buffer
                            if( f_pre_trigger_buffer.full() )
                            {
                                LTRACE( plog, "Current state untriggered. Writing id "<<f_pre_trigger_buffer.front()<<" as false");
                                if( ! write_output_from_prebuff_front( false, t_write_flag ) )
                                {
                                    goto exit_outer_loop;
                                }
                                // pretrigger buffer is full - 1
                            }
                        }
                    }
                    else if (f_state == state_t::possibly_triggered)
                    {
                        // wait for major trigger. otherwise return to untriggered
                        if (t_current_trig_flag)
                        {
                            f_state = state_t::triggered;
                            LDEBUG( plog, "Next state is triggered");
                        }
                        else if( f_event_buffer.full() )
                        {
                            // set state to untriggered
                            f_state = state_t::untriggered;
                            LDEBUG( plog, "Next state is untriggered" );
                            if (f_event_buffer.size() >= f_pre_trigger_buffer.capacity())
                            {
                                while( ! f_pre_trigger_buffer.empty() )
                                {
                                    LTRACE( plog, "Next state untriggered. Writing id "<<f_pre_trigger_buffer.front()<<" as false");
                                    if( ! write_output_from_prebuff_front( false, t_write_flag ) )
                                    {
                                        goto exit_outer_loop;
                                    }
                                    // advance our output data pointer to the next in the stream
                                    t_write_flag = out_stream< 0 >().data();
                                }
                                // empty skip buffer, write as false and fill pretrigger buffer
                                while( f_event_buffer.size() >= f_pre_trigger_buffer.capacity() )
                                {
                                    LTRACE( plog, "Next state untriggered. Writing id "<<f_event_buffer.front()<<" as false");
                                    if( ! write_output_from_ebuff_front( false, t_write_flag ) )
                                    {
                                        goto exit_outer_loop;
                                    }
                                    // advance our output data pointer to the next in the stream
                                    t_write_flag = out_stream< 0 >().data();
                                }
                                //
                                while( ! f_event_buffer.empty())
                                {
                                    LTRACE(plog, "Writing event buffer front: "<<f_event_buffer.front());
                                    f_pre_trigger_buffer.push_back(f_event_buffer.front());
                                    LTRACE(plog, "to pt buffer back: "<<f_pre_trigger_buffer.back());
                                    f_event_buffer.pop_front();
                                }
                                LTRACE( plog, "Finished moving IDs. Capacities and sizes are (pre/skip): "<<f_pre_trigger_buffer.capacity()<<"/"<<f_pre_trigger_buffer.size()<<" "<<f_event_buffer.capacity()<<"/"<<f_event_buffer.size());
                            }
                            else
                            {
                                while( f_pre_trigger_buffer.capacity() <= f_event_buffer.size() + f_pre_trigger_buffer.size() )
                                {
                                    if( ! write_output_from_ebuff_front( false, t_write_flag ) )
                                    {
                                        goto exit_outer_loop;
                                    }
                                    // advance our output data pointer to the next in the stream
                                    t_write_flag = out_stream< 0 >().data();
                                }
                                while( !f_event_buffer.empty() )
                                {
                                    f_pre_trigger_buffer.push_back(f_event_buffer.front());
                                    f_event_buffer.pop_front();
                                }
                            }
                        }
                    }
                    else if( f_state == state_t::post_triggered )
                    {
                        LTRACE( plog, "Currently in post_triggered state" );

                        if( t_current_trig_flag )
                        {
                            f_state = state_t::triggered;
                        }
                        else
                        {
                            if(f_post_trigger_buffer.full() )
                            {
                                f_state = state_t::untriggered;
                                LINFO( plog, "Skip_tolerance reached. Continuing as untriggered");
                                // if skip buffer is not bigger than pretrigger buffer, write out ids as true
                                if ( f_post_trigger_buffer.capacity() <= f_pre_trigger_buffer.capacity() )
                                {
                                    while( ! f_post_trigger_buffer.empty() )
                                    {
                                        if( ! write_output_from_postbuff_front( true, t_write_flag ) )
                                        {
                                            goto exit_outer_loop;
                                        }
                                        // advance our output data pointer to the next in the stream
                                        t_write_flag = out_stream< 0 >().data();
                                    }
                                    // now all buffers are empty
                                }
                                // else only write out what doesn't fit into pre_trigger_buffer
                                else
                                {
                                    while( f_post_trigger_buffer.size() >= f_pre_trigger_buffer.capacity() )
                                    {
                                        if( ! write_output_from_postbuff_front( true, t_write_flag ) )
                                        {
                                            goto exit_outer_loop;
                                        }
                                        // advance our output data pointer to the next in the stream
                                        t_write_flag = out_stream< 0 >().data();
                                    }
                                    while( ! f_post_trigger_buffer.empty() )
                                    {
                                        f_pre_trigger_buffer.push_back(f_post_trigger_buffer.front());
                                        f_post_trigger_buffer.pop_front();
                                    }
                                    // now post_trigger_buffer is empty and pre_trigger_buffer has space for one id
                                }
                            }
                        }
                    }
                    if( f_state == state_t::triggered )
                    {
                        // empty buffers
                        while( ! f_pre_trigger_buffer.empty() )
                        {
                            if( ! write_output_from_prebuff_front( true, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
                        while( ! f_event_buffer.empty())
                        {
                            if( ! write_output_from_ebuff_front( true, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
                        while( ! f_post_trigger_buffer.empty() )
                        {
                            if( ! write_output_from_postbuff_front( true, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
                        f_state = state_t::post_triggered;
                    }


                } // end if( t_in_command == stream::s_run )


                if( t_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "Event builder is stopping at stream index " << out_stream< 0 >().get_current_index() );
                    LDEBUG( plog, "Flushing buffers as untriggered" );

                    while( ! f_pretrigger_buffer.empty() and ! f_skip_buffer.empty() )
                    {
                        if( f_pretrigger_buffer.front() == f_skip_buffer.front() )
                        {
                            LTRACE( plog, "Skip id " << f_skip_buffer.front() );
                            if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            t_write_flag = out_stream< 0 >().data();
                            f_pretrigger_buffer.pop_front();
                        }
                        else if( f_pretrigger_buffer.front() < f_skip_buffer.front() )
                        {
                            LTRACE( plog, "Pretrigger id "<<f_pretrigger_buffer.front());
                            if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            t_write_flag = out_stream< 0 >().data();
                        }
                        else
                        {
                            LTRACE( plog, "Skip id "<<f_skip_buffer.front() );
                            if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            t_write_flag = out_stream< 0 >().data();
                        }
                    }

                    if( f_skip_buffer.empty() )
                    {
                        LDEBUG( plog, "Skip buffer is empty" );
                        while( ! f_pretrigger_buffer.empty() )
                        {
                            LTRACE( plog, "Pretrigger id " << f_pretrigger_buffer.front() );
                            if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                            {   
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
                    }
                    else if (f_pretrigger_buffer.empty() )
                    {
                        LDEBUG( plog, "Pretrigger buffer is empty");
                        while( ! f_skip_buffer.empty() )
                        {
                            LTRACE( plog, "Skip id "<<f_skip_buffer.front() );
                            if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                            {   
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
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
                    LDEBUG( plog, "Flushing buffers as untriggered" );

                    while( ! f_pretrigger_buffer.empty() and !f_skip_buffer.empty() )
                    {
                        if( f_pretrigger_buffer.front() == f_skip_buffer.front() )
                        {
                            LTRACE( plog, "Skip id "<<f_skip_buffer.front() );
                            if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                            {   
                                goto exit_outer_loop;
                            }
                            t_write_flag = out_stream< 0 >().data();
                            f_pretrigger_buffer.pop_front();
                        }
                        else if( f_pretrigger_buffer.front() < f_skip_buffer.front() )
                        {
                            LTRACE( plog, "Pretrigger id "<<f_pretrigger_buffer.front() );
                            if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                            {   
                                goto exit_outer_loop;
                            }
                            t_write_flag = out_stream< 0 >().data();
                        }
                        else
                        {
                            LTRACE( plog, "Skip id "<<f_skip_buffer.front() );
                            if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                            {   
                                goto exit_outer_loop;
                            }
                            t_write_flag = out_stream< 0 >().data();
                        }
                    }
                    if( f_skip_buffer.empty())
                    {
                        LTRACE( plog, "Skip buffer is empty" );
                        while( !f_pretrigger_buffer.empty())
                        {
                            LTRACE( plog, "Pretrigger id "<<f_pretrigger_buffer.front() );
                            if( ! write_output_from_ptbuff_front( false, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
                    }
                    else if( f_pretrigger_buffer.empty() )
                    {
                        LTRACE( plog, "Pretrigger buffer is empty" );
                        while( !f_skip_buffer.empty() )
                        {
                            LTRACE( plog, "Skip id "<<f_skip_buffer.front() );
                            if( ! write_output_from_skipbuff_front( false, t_write_flag ) )
                            {
                                goto exit_outer_loop;
                            }
                            // advance our output data pointer to the next in the stream
                            t_write_flag = out_stream< 0 >().data();
                        }
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
        a_node->set_n_triggers( a_config.get_value( "n-triggers", a_node->get_n_triggers() ) );
        return;
    }

    void event_builder_binding::do_dump_config( const event_builder* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for event_builder" );
        a_config.add( "length", scarab::param_value( a_node->get_length() ) );
        a_config.add( "pretrigger", scarab::param_value( a_node->get_pretrigger() ) );
        a_config.add( "skip-tolerance", scarab::param_value( a_node->get_skip_tolerance() ) );
        a_config.add( "n-triggers", scarab::param_value( a_node->get_n_triggers() ) );
        return;
    }


} /* namespace psyllid */
