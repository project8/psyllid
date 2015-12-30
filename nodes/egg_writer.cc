/*
 * egg_writer.cc
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#include "egg_writer.hh"

namespace psyllid
{

    egg_writer::egg_writer()
    {
    }

    egg_writer::~egg_writer()
    {
    }

    void egg_writer::initialize()
    {
        return;
    }

    void egg_writer::execute()
    {
        enum_t t_event_command = stream::s_none;
        enum_t t_time_command = stream::s_none;

        id_range_event* t_event_data = nullptr;
        time_data* t_time_data = nullptr;

        while( true )
        {
            /*
             * In most cases the time and event commands should be matched.
             * There is one exception: the event stream could be stopped (after it reaches its last triggered event)
             * while the time stream is still running.
             * Therefore we'll check if the event command is s_stop first, before comparing the time and event commands.
             */

            t_time_command = in_stream< 0 >().get();
            t_event_command = in_stream< 1 >().get();

            if( t_event_command == stream::s_stop )
            {
                pmsg( s_debug ) << "Event stream has stopped; advancing the time stream" << eom;

                while( t_time_command != stream::s_stop )
                {
                    t_time_command = in_stream< 0 >().get();
                }

                pmsg( s_debug ) << "Closing the egg file" << eom;

                //TODO: close the egg file

                continue;
            }

            if( t_time_command != t_event_command )
            {
                throw error() << "Time and event stream commands are mismatched: " << t_time_command << " (time) vs " << t_event_command << " (event)";
            }

            t_time_data = in_stream< 0 >().data();
            t_event_data = in_stream< 1 >().data();

            if( t_event_command == stream::s_start )
            {
                pmsg( s_debug ) << "Preparing egg file" << eom;

                //TODO: prepare egg file

                continue;
            }

            if( t_event_command == stream::s_run )
            {
                pmsg( s_debug ) << "Writing an event to the egg file" << eom;

                count_t t_time_id = t_time_data->get_id();
                if( t_time_id != t_event_data->get_start_id() )
                {
                    throw error() << "Time stream is not at the start of the current event";
                }

                count_t t_event_end_id = t_event_data->get_end_id();
                while( true )
                {
                    //TODO: write time data t_time_id to file

                    if( t_time_id == t_event_end_id ) break;

                    t_time_command = in_stream< 0 >().get();
                    if( t_time_command != stream::s_run )
                    {
                        throw error() << "Time stream stopped mid-event; time_id: " << t_time_id;
                    }

                    t_time_data = in_stream< 0 >().data();
                    t_time_id = t_time_data->get_id();
                }

                continue;
            }

            if( t_event_command == stream::s_exit )
            {
                return;
            }
        }

        return;
    }

    void egg_writer::finalize()
    {
        return;
    }


} /* namespace psyllid */
