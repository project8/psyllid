/*
 * egg_writer.cc
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#include "egg_writer.hh"

#include "psyllidmsg.hh"

#include "M3Monarch.hh"

using namespace monarch3;

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

        Monarch3* t_monarch = nullptr;
        M3Header* t_monarch_header = nullptr;
        M3Stream* t_monarch_stream = nullptr;
        M3Record* t_monarch_record = nullptr;

        unsigned t_stream_num = 0;
        count_t t_bytes_per_record = 0;
        bool t_is_new_event = true;

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

                t_monarch->FinishWriting();

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
                string t_filename( "test_file.egg" );
                t_monarch = Monarch3::OpenForWriting( t_filename );
                if( t_monarch == nullptr )
                {
                    throw error() << "Unable to open the egg file <" << t_filename << ">";
                }

                t_monarch_header = t_monarch->GetHeader();
                // to set:
                //   run duration
                //   timestamp
                //   description


                vector< unsigned > t_chan_vec;
                t_stream_num = t_monarch_header->AddStream( "Psyllid - ROACH2", 100, 1, 1, 1, sDigitizedUS, 8, sBitsAlignedLeft, &t_chan_vec );

                unsigned i_chan_psyllid = 0; // this is the channel number in mantis, as opposed to the channel number in the monarch file
                for( std::vector< unsigned >::const_iterator it = t_chan_vec.begin(); it != t_chan_vec.end(); ++it )
                {
                    t_monarch_header->GetChannelHeaders()[ *it ].SetVoltageOffset( 0. );
                    t_monarch_header->GetChannelHeaders()[ *it ].SetVoltageRange( 0.5 );
                    t_monarch_header->GetChannelHeaders()[ *it ].SetDACGain( 1. );
                    ++i_chan_psyllid;
                }

                t_bytes_per_record = t_time_data->get_array_n_bytes();

                t_monarch_stream = t_monarch->GetStream( 0 );
                t_monarch_record = t_monarch_stream->GetStreamRecord();

                continue;
            }

            if( t_event_command == stream::s_run )
            {
                pmsg( s_debug ) << "Writing an event to the egg file" << eom;
                t_is_new_event = true;

                count_t t_time_id = t_time_data->get_id();
                if( t_time_id != t_event_data->get_start_id() )
                {
                    throw error() << "Time stream is not at the start of the current event";
                }

                count_t t_event_end_id = t_event_data->get_end_id();
                while( true )
                {
                    memcpy( t_monarch_record->GetData(), t_time_data->array()->data(), t_bytes_per_record );
                    t_monarch_stream->WriteRecord( t_is_new_event );

                    if( t_time_id == t_event_end_id ) break;

                    t_time_command = in_stream< 0 >().get();
                    if( t_time_command != stream::s_run )
                    {
                        throw error() << "Time stream stopped mid-event; time_id: " << t_time_id;
                    }

                    t_time_data = in_stream< 0 >().data();
                    t_time_id = t_time_data->get_id();

                    t_is_new_event = false;
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
