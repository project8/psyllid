/*
 * locust_egg_reader.cc
 *
 *  Created on: Dec. 13, 2024
 *      Author: pkolbeck
 */


#include "locust_egg_reader.hh"

#include "psyllid_error.hh"
#include "time_data.hh"

#include "M3Monarch.hh"

#include "run_control.hh"

#include "logger.hh"
#include "param.hh"

#include <chrono>
#include <bitset>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( locust_egg_reader, "locust-egg-reader", locust_egg_reader_binding );

    LOGGER( plog, "locust_egg_reader" );

    // locust_egg_reader methods
    locust_egg_reader::locust_egg_reader() :
            f_egg( nullptr ),
            f_egg_path( "/dev/null" ),
            f_read_n_records( 0 ),
            f_repeat_egg( false ),
            f_length( 10 ),
            f_start_paused( true ), 
            f_slice_length( PAYLOAD_SIZE ),
            f_paused( true ),
            f_record_length( 0 ),
            f_sample_size( 0 ),
            f_pkt_id_offset( 0 ),
            f_uint_to_int( false )
    {
    }

    locust_egg_reader::~locust_egg_reader()
    {
        cleanup_file();
    }

    void locust_egg_reader::initialize()
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
        f_record_length = t_egg_header->ChannelHeaders()[0].GetRecordSize();
        f_sample_size = t_egg_header->ChannelHeaders()[0].GetSampleSize();
        return;

    }

    void locust_egg_reader::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the locust_egg_reader" );
            LDEBUG( plog, "Uint to int conversion: [" << f_uint_to_int << "]")
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

            uint64_t t_records_read = 0;
            uint64_t t_slice_offset = 0;

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
                        t_records_read = 0;
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
                    //if ( !read_slice(t_data, t_stream, t_record) ) break;
                    bool write_slice_ok = write_slice(t_data, t_stream, t_record, &t_slice_offset, &t_records_read);
                    if ( !write_slice_ok || (f_read_n_records > 0 && t_records_read >= f_read_n_records) )
                    {
                        LINFO( plog, "breaking out of loop because record limit or end of file reached" );
                        std::shared_ptr< sandfly::run_control > t_run_control = use_run_control();
                        t_run_control->stop_run();
                    }
                    // add some sleep to try and not lap downstream nodes
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
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
        LDEBUG( plog, "at the end of locust egg execute" );
    }

    void locust_egg_reader::finalize()
    {
        LDEBUG( plog, "finalize the locust_egg_reader" );
        out_buffer< 0 >().finalize();
        LDEBUG( plog, "buffer finalized" );
        cleanup_file();
        return;
    }

    bool locust_egg_reader::read_slice(time_data* t_data, const monarch3::M3Stream* t_stream, const monarch3::M3Record* t_record)
    {
        LDEBUG( plog, "reading a slice" );
        // update t_data to point to the next slot in the output stream
        t_data = out_stream< 0 >().data();
        // read next record in egg file, writing into the output_stream
        if ( !t_stream->ReadRecord() )
        {
            if ( !f_repeat_egg )
            {
                LDEBUG( plog, "reached end of file, stopping" );
                return false;
            }
            else
            {
                LDEBUG( plog, "reached end of file, restarting" );
                t_stream->ReadRecord( -1 * int(t_stream->GetRecordCountInFile()) );
                // when we loop back, we want the record ID to increment and have a gap relative to the end of the file
                f_pkt_id_offset += 1 + t_stream->GetNRecordsInFile();
            }
        }
        std::copy(&t_record->GetData()[0], &t_record->GetData()[f_record_length*2], &t_data->get_array()[0][0]);

        // packet ID logic
        //TODO do this pkt ID logic reasonable?
        t_data->set_pkt_in_batch( t_record->GetRecordId() + f_pkt_id_offset );
        t_data->set_pkt_in_session( t_record->GetRecordId() + f_pkt_id_offset );
        if ( !out_stream< 0 >().set( stream::s_run ) )
        {
            LERROR( plog, "egg reader exiting due to stream error" );
            return false;
        }
        return true;
    }

    bool locust_egg_reader::read_record( const monarch3::M3Stream* t_stream ) 
    {
        LDEBUG( plog, "attempting to read a record")
        if ( !t_stream->ReadRecord() )
        {
            if ( !f_repeat_egg )
            {
                LDEBUG( plog, "reached end of file, stopping" );
                return false;
            }
            else
            {
                LDEBUG( plog, "reached end of file, restarting" );
                t_stream->ReadRecord( -1 * int(t_stream->GetRecordCountInFile()) );
                // when we loop back, we want the record ID to increment and have a gap relative to the end of the file
                f_pkt_id_offset += 1 + t_stream->GetNRecordsInFile();
            }
        }
        return true;
    }

    void locust_egg_reader::packet_logic( time_data* t_data, const monarch3::M3Record* t_record )
    {
        // packet ID logic
        //TODO do this pkt ID logic reasonable?
        t_data->set_pkt_in_batch( t_record->GetRecordId() + f_pkt_id_offset );
        t_data->set_pkt_in_session( t_record->GetRecordId() + f_pkt_id_offset );
    }

    bool locust_egg_reader::check_stream()
    {
        if ( !out_stream< 0 >().set( stream::s_run ) )
        {
            LERROR( plog, "egg reader exiting due to stream error" );
            return false;
        }
        return true;
    }

    void locust_egg_reader::convert_uiq_to_iq( const u_char* t_source, int8_t* t_target, int t_data_len, bool t_convert )
    {
        uint8_t t_128 = 128;
        if( t_convert )
        {
            for(int i = 0; i < t_data_len; i++ )
            {
                t_target[i] = t_source[i] ^ t_128;
            }
        }
        else
        {
            std::copy(t_source, &t_source[t_data_len], t_target);
        }

    }

    bool locust_egg_reader::write_slice( time_data* t_data, const monarch3::M3Stream* t_stream, const monarch3::M3Record* t_record, uint64_t* t_slice_offset, uint64_t* t_records_read )
    {
        uint32_t t_num_samples = f_record_length * f_sample_size;
        if ( f_slice_length > t_num_samples)
        {
            LERROR( plog, "slice length is longer than record length * sample size");
            return false;
        }

        // update t_data to point to the next slot in the output stream. 
        t_data = out_stream< 0 >().data();



        // starting
        if( *t_records_read == 0 )
        {
            LDEBUG( plog, "starting to read the first record" );
            // read record
            if ( !read_record( t_stream ) )
            {
                // end of file
                LDEBUG( plog, "reached end of file" );
                return false;
            }
            // have started reading another record.
            (*t_records_read)++;
        
        }

        LDEBUG( plog,  "writing slice of length [" << f_slice_length << "] at offset [" << *t_slice_offset << "] in record with [" << t_num_samples << "] numbers");
        if( *t_slice_offset + f_slice_length <= t_num_samples )
        {
            // don't have to read data from the next record yet. 
            // copy the part of the new record
            convert_uiq_to_iq(&t_record->GetData()[*t_slice_offset], &t_data->get_array()[0][0], f_slice_length, f_uint_to_int);
            // packet logic
            packet_logic( t_data, t_record );
            // check stream
            if ( !check_stream() )
            {
                return false;
            }
            // increment t_slice_offset by f_slice_length
            *t_slice_offset += f_slice_length;
        }
        else
        {
            // have to read data from the next record if want to continue

            if( *t_records_read >= f_read_n_records )
            {
                // we've reached the specified max number of records to read
                LDEBUG( plog, "reached max records" );
                return false;
            }

            // keep pointer to current record
            const monarch3::M3Record* t_old_record = t_record;
            // read new record
            if( !read_record( t_stream ) )
            {
                // end of file
                LDEBUG( plog, "reached end of file" );
                return false; 
            }
            // have started reading another record
            (*t_records_read)++;

            LDEBUG( plog, "switching to new record" );
            // get amount left in current record
            uint64_t t_num_samples_left = t_num_samples - *t_slice_offset;
            // copy rest of current data
            convert_uiq_to_iq(&t_old_record->GetData()[*t_slice_offset], &t_data->get_array()[0][0], t_num_samples_left, f_uint_to_int);
            // update slice offset
            *t_slice_offset = f_slice_length - t_num_samples_left;
            // copy new data
            convert_uiq_to_iq(&t_record->GetData()[0], &t_data->get_array()[0][t_num_samples_left], *t_slice_offset, f_uint_to_int);

            // packet logic
            packet_logic( t_data, t_record );
            // check stream
            if ( !check_stream() )
            {
                return false;
            }

        }

        return true;

    }

    void locust_egg_reader::cleanup_file()
    {
        LDEBUG( plog, "cleaning up file" );
        if ( f_egg == NULL ) return;
        LDEBUG( plog, "clean egg" );
        if ( f_egg->GetState() != monarch3::Monarch3::eClosed )
        {
            LDEBUG( plog, "actually close egg" );
            f_egg->FinishReading();
        }
    }

    // locust_egg_reader_binding methods
    locust_egg_reader_binding::locust_egg_reader_binding() :
            sandfly::_node_binding< locust_egg_reader, locust_egg_reader_binding >()
    {
    }

    locust_egg_reader_binding::~locust_egg_reader_binding()
    {
    }

    void locust_egg_reader_binding::do_apply_config( locust_egg_reader* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring locust_egg_reader with:\n" << a_config );
        a_node->set_egg_path( a_config.get_value( "egg-path", a_node->get_egg_path() ) );
        a_node->set_read_n_records( a_config.get_value( "read-n-records", a_node->get_read_n_records() ) );
        a_node->set_repeat_egg( a_config.get_value( "repeat-egg", a_node->get_repeat_egg() ) );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        a_node->set_start_paused( a_config.get_value( "start-paused", a_node->get_start_paused() ) );
        // a_node->set_slice_length( a_config.get_value( "slice-length", a_node->get_slice_length() ) );
        a_node->set_uint_to_int( a_config.get_value( "uint-to-int", a_node->get_uint_to_int() ) );
        return;
    }

    void locust_egg_reader_binding::do_dump_config( const locust_egg_reader* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for locust_egg_reader" );
        a_config.add( "egg-path", scarab::param_value( a_node->get_egg_path() ) );
        a_config.add( "read-n-records", scarab::param_value( a_node->get_read_n_records() ) );
        a_config.add( "repeat-egg", scarab::param_value( a_node->get_repeat_egg() ) );
        a_config.add( "length", scarab::param_value( a_node->get_length() ) );
        a_config.add( "start-paused", scarab::param_value( a_node->get_length() ) );
        // a_config.add( "slice-length", scarab::param_value( a_node->get_slice_length() ) );
        a_config.add( "uint-to-int", scarab::param_value( a_node->get_uint_to_int() ) );
        return;
    }

} /* namespace psyllid */
