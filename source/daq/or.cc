/*
 * or.cc
 *
 *  Created on: Aug 31, 2018
 *      Author: cclaessens
 */

#include "or.hh"
#include "psyllid_error.hh"
#include "digital.hh"
#include "logger.hh"
#include "time.hh"
#include <limits>

using midge::stream;

using std::string;
using std::vector;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( or_node, "or_node", or_node_binding );

    LOGGER( orlog, "or_node" );

    or_node::or_node()
    {
    }

    or_node::~or_node()
    {
    }


    void or_node::initialize()
    {
        return;
    }

    void or_node::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_in_command = stream::s_none;
            midge::enum_t t_in_command_1 = stream::s_none;
            trigger_flag* t_in_trigger_flag_0 = nullptr;
            trigger_flag* t_in_trigger_flag_1 = nullptr;
            trigger_flag* t_out_trigger_flag = nullptr;

            bool t_current_trig_flag_0 = false;
            bool t_current_trig_flag_1 = false;
            bool t_current_trig_flag_out = false;

            while( ! is_canceled() )
            {
                t_in_command = in_stream< 0 >().get();
                t_in_command_1 = in_stream< 1 >().get();
                if( t_in_command == stream::s_none ) continue;
                if( t_in_command == stream::s_error ) break;
                if( t_in_command == stream::s_exit ) break;

                t_in_trigger_flag_0 = in_stream< 0 >().data();
                t_in_trigger_flag_1 = in_stream< 1 >().data();
                t_out_trigger_flag = out_stream< 0 >().data();


                if( t_in_command == stream::s_start )
                {
                    LDEBUG( orlog, "Starting the or_node" );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    continue;
                }
                if( t_in_command == stream::s_run )
                {
                    if( ( t_in_command_1 == stream::s_none ) or ( t_in_command_1 == stream::s_error ) or ( t_in_command_1 == stream::s_exit ))
                        {
                            LERROR( orlog, "Rececived "<< t_in_command_1 << " from in_stream<1> while running" );
                            goto exit_outer_loop;
                        }
                    uint64_t t_trig_id_0 = t_in_trigger_flag_0->get_id();
                    uint64_t t_trig_id_1 = t_in_trigger_flag_1->get_id();
                    if( t_trig_id_0 != t_trig_id_1 )
                    {
                        LERROR( orlog, "Mismatch between id 0<" << t_trig_id_0 << "> and id 1<" << t_trig_id_1 << ">" );
                        throw midge::node_nonfatal_error() << "Unable to match time and trigger streams";
                    }
                    t_current_trig_flag_0 = t_in_trigger_flag_0->get_flag();
                    t_current_trig_flag_1 = t_in_trigger_flag_0->get_flag();
                    t_current_trig_flag_out = t_current_trig_flag_0 or t_current_trig_flag_1;

                    t_out_trigger_flag->set_id( t_trig_id_0 );
                    t_out_trigger_flag->set_flag( t_current_trig_flag_out );

                    if( ! out_stream< 0 >().set( midge::stream::s_run ) )
                    {
                        LERROR( orlog, "Exiting due to stream error" );
                        goto exit_outer_loop;
                    }
                }
                if( t_in_command == stream::s_stop )
                {
                    LDEBUG( orlog, "Or node is stopping at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_stop ) )
                    {
                        LERROR( orlog, "Exiting due to stream error" );
                        break;
                    }
                    continue;
                }

                if( t_in_command == stream::s_exit )
                {
                    LDEBUG( orlog, "Event builder is exiting at stream index " << out_stream< 0 >().get_current_index() );

                    out_stream< 0 >().set( stream::s_exit );
                    break;
                }

            } // end while( ! is_canceled() )

exit_outer_loop:
            LDEBUG( orlog, "Stopping output stream" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( orlog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );
        }

        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }


    void or_node::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }


    or_node_binding::or_node_binding() :
            _node_binding< or_node, or_node_binding >()
    {
    }

    or_node_binding::~or_node_binding()
    {
    }

    void or_node_binding::do_apply_config( or_node* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( orlog, "Configuring or_node with:\n" << a_config );
        return;
    }

    void or_node_binding::do_dump_config( const or_node* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( orlog, "Dumping configuration for or_node" );
        return;
    }


} /* namespace psyllid */
