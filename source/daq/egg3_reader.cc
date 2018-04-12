/*
 * egg3_reader.cc
 *
 *  Created on: Dec. 14, 2017
 *      Author: laroque
 */

#include <chrono>

#include "egg3_reader.hh"
#include "psyllid_error.hh"
#include "time_data.hh"
#include "M3Monarch.hh"

#include "logger.hh"
#include "M3Monarch.hh"
#include "param.hh"

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( egg3_reader, "egg3-reader", egg3_reader_binding );

    LOGGER( plog, "egg3_reader" );

    // egg3_reader methods
    egg3_reader::egg3_reader() :
            f_egg( nullptr ),
            f_egg_path( "/dev/null" ),
            f_length( 10 ),
            f_start_paused( true ),
            f_paused( true ),
            f_record_length( 0 )
    {
    }

    egg3_reader::~egg3_reader()
    {
        cleanup_file();
    }

    void egg3_reader::initialize()
    {
        f_paused = f_start_paused;

        out_buffer< 0 >().initialize( f_length );

        LDEBUG( plog, "opening egg file [" << f_egg_path << "]" );
        f_egg = monarch3::Monarch3::OpenForReading( f_egg_path );
        f_egg->ReadHeader();
        // do we want/need to do anything with the header?
        const monarch3::M3Header *t_egg_header = f_egg->GetHeader();
        LDEBUG( plog, "egg header content:\n" );
        LDEBUG( plog, *t_egg_header );
        //TODO this should probably not assume single-channel mode...
        f_record_length = t_egg_header->GetChannelHeaders()[0].GetRecordSize();
        return;

    }

    void egg3_reader::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the egg3_reader" );
            //TODO  use header to loop streams so we can send more than one?
            //const monarch3::M3Header *t_egg_header = f_egg->GetHeader();
            const monarch3::M3Stream* t_stream = f_egg->GetStream( 0 );
            const monarch3::M3Record* t_record = t_stream->GetChannelRecord( 0 );

            time_data* t_data = nullptr;

            // starting not in a paused state is not currently known to work
            if ( !f_paused )
            {
                if( ! out_stream< 0 >().set( stream::s_start ) ) return;
            }

            // starting execution loop
            while (! is_canceled() )
            {
                if( (out_stream< 0 >().get() == stream::s_stop) )
                {
                    LWARN( plog, "Output stream(s) have stop condition" );
                    break;
                }
                if( have_instruction() )
                {
                    if( f_paused && use_instruction() == midge::instruction::resume )
                    {
                        LDEBUG( plog, "egg reader resuming" );
                        if( ! out_stream< 0 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 0 error while starting";
                        f_paused = false;
                    }
                    else if ( !f_paused && use_instruction() == midge::instruction::pause )
                    {
                        LDEBUG( plog, "egg reader pausing" );
                        if( ! out_stream< 0 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 0 error while stopping";
                        f_paused = true;
                    }
                }
                // only read if not paused:
                if ( ! f_paused )
                {
                    LDEBUG( plog, "not paused, reading slice" );
                    if ( !read_slice(t_data, t_stream, t_record) ) break;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
        catch( std::exception )
        {
            LWARN( plog, "got an exception, throwing" );
            a_midge->throw_ex( std::current_exception() );
        }
        LWARN( plog, "at the end of egg3 execute" );
    }

    void egg3_reader::finalize()
    {
        LDEBUG( plog, "finalize the egg3_reader" );
        out_buffer< 0 >().finalize();
        cleanup_file();
    }

    bool egg3_reader::read_slice(time_data* t_data, const monarch3::M3Stream* t_stream, const monarch3::M3Record* t_record)
    {
        LDEBUG( plog, "reading a slice" );
        // update t_data to point to the next slot in the output stream
        t_data = out_stream< 0 >().data();
        // update M3Record so that it will write into t_data... this doesn't currently work
        //t_record->UpdateDataPtr( reinterpret_cast< const monarch3::byte_type* >(t_data->get_raw_array()) );
        // read next record in egg file, writing into the output_stream
        if ( !t_stream->ReadRecord() )
        {
            LDEBUG( plog, "reached end of file" );
            return false;
        }
        std::copy(&t_record->GetData()[0], &t_record->GetData()[f_record_length*2], &t_data->get_array()[0][0]);

        // packet ID logic
        //TODO do this pkt ID logic reasonable?
        t_data->set_pkt_in_batch(t_record->GetRecordId());
        t_data->set_pkt_in_session(t_record->GetRecordId());
        if ( !out_stream< 0 >().set( stream::s_run ) )
        {
            LERROR( plog, "egg reader exiting due to stream error" );
            return false;
        }
        return true;
    }

    void egg3_reader::cleanup_file()
    {
        LDEBUG( plog, "clean egg" );
        if ( f_egg->GetState() != monarch3::Monarch3::eClosed )
        {
            LDEBUG( plog, "actually close egg" );
            f_egg->FinishReading();
        }
    }

    // egg3_reader_binding methods
    egg3_reader_binding::egg3_reader_binding() :
            _node_binding< egg3_reader, egg3_reader_binding >()
    {
    }

    egg3_reader_binding::~egg3_reader_binding()
    {
    }

    void egg3_reader_binding::do_apply_config( egg3_reader* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring egg3_reader with:\n" << a_config );
        a_node->set_egg_path( a_config.get_value( "egg-path", a_node->get_egg_path() ) );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        a_node->set_start_paused( a_config.get_value( "start-paused", a_node->get_start_paused() ) );
        return;
    }

    void egg3_reader_binding::do_dump_config( const egg3_reader* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for egg3_reader" );
        a_config.add( "egg-path", new scarab::param_value( a_node->get_egg_path() ) );
        a_config.add( "length", new scarab::param_value( a_node->get_length() ) );
        a_config.add( "start-paused", new scarab::param_value( a_node->get_length() ) );
        return;
    }

} /* namespace psyllid */
