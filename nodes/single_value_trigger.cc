/*
 * single_value_trigger.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#include "single_value_trigger.hh"

#include "psyllidmsg.hh"

namespace psyllid
{

    single_value_trigger::single_value_trigger() :
            f_length( 10 ),
            f_threshold( 0.7 )
    {
    }

    single_value_trigger::~single_value_trigger()
    {
    }

    void single_value_trigger::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void single_value_trigger::execute()
    {
        midge::enum_t t_in_command = stream::s_none;
        freq_data* t_freq_data = nullptr;
        trigger_flag* t_trigger_flag = nullptr;

        while( true )
        {
            t_in_command = in_stream< 0 >().get();
            t_freq_data = in_stream< 0 >().data();
            t_trigger_flag = out_stream< 0 >().data();

            if( t_in_command == stream::s_start )
            {
                out_stream< 0 >().set( stream::s_start );
                continue;
            }

            if( t_in_command == stream::s_run )
            {
                t_trigger_flag->set_flag( (*t_freq_data->array())[0] >= f_threshold );
                t_trigger_flag->set_id( t_freq_data->get_id() );

                pmsg( s_debug ) << "Data " << t_trigger_flag->get_id() << " at " << (*t_freq_data->array())[0] << " resulted in flag <" << t_trigger_flag->get_flag() << ">" << eom;

                out_stream< 0 >().set( stream::s_run );
                continue;
            }

            if( t_in_command == stream::s_stop )
            {
                out_stream< 0 >().set( stream::s_stop );
                continue;
            }

            if( t_in_command == stream::s_exit )
            {
                out_stream< 0 >().set( stream::s_exit );
                break;
            }
        }

    }

    void single_value_trigger::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }


} /* namespace psyllid */
