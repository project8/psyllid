/*
 * monarch3_wrap.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 */

#include "monarch3_wrap.hh"

#include "butterfly_house.hh"
#include "psyllid_constants.hh"
#include "psyllid_error.hh"

#include "logger.hh"


namespace psyllid
{
    LOGGER( plog, "monarch3_wrap" );


    uint32_t to_uint( monarch_stage a_stage )
    {
        return static_cast< uint32_t >( a_stage );
    }
    monarch_stage to_op_t( uint32_t a_stage_uint )
    {
        return static_cast< monarch_stage >( a_stage_uint );
    }
    std::ostream& operator<<( std::ostream& a_os, monarch_stage a_stage )
    {
        return a_os << to_uint( a_stage );
    }


    header_wrapper::header_wrapper( monarch3::Monarch3& a_monarch, std::mutex& a_mutex ) :
        f_header( a_monarch.GetHeader() ),
        f_lock( a_mutex ),
        f_global_setup_done( false )
    {
        if( ! f_header )
        {
            throw error() << "Unable to get monarch header";
        }
    }

    header_wrapper::header_wrapper( header_wrapper&& a_orig ) :
            f_header( a_orig.f_header ),
            f_lock( *a_orig.f_lock.release() ),
            f_global_setup_done( a_orig.f_global_setup_done )
    {
        a_orig.f_header = nullptr;
        a_orig.f_global_setup_done = false;
    }

    header_wrapper::~header_wrapper()
    {
        // lock is unlocked upon destruction
    }

    header_wrapper& header_wrapper::operator=( header_wrapper&& a_orig )
    {
        f_header = a_orig.f_header;
        a_orig.f_header = nullptr;
        f_lock.release();
        f_lock.swap( a_orig.f_lock );
        f_global_setup_done = a_orig.f_global_setup_done;
        a_orig.f_global_setup_done = false;
        return *this;
    }

    monarch3::M3Header& header_wrapper::header()
    {
        if( ! f_header ) throw error() << "Unable to write to header; the owning Monarch object must have moved beyond the preparation stage";
        return *f_header;
    }

/*
    void header_wrapper::monarch_stage_change( monarch_stage a_new_stage )
    {
        if( a_new_stage != monarch_stage::preparing )
        {
            f_header = nullptr;
            f_lock.release();
        }
        return;
    }
*/



    stream_wrapper::stream_wrapper( monarch3::Monarch3& a_monarch, unsigned a_stream_no ) :
        f_stream( a_monarch.GetStream( a_stream_no ) ),
        f_is_valid( true )
    {
        if( f_stream == nullptr )
        {
            throw error() << "Invalid stream number requested: " << a_stream_no;
        }
    }

    stream_wrapper::stream_wrapper( stream_wrapper&& a_orig ) :
            f_stream( a_orig.f_stream ),
            f_is_valid( a_orig.f_is_valid )
    {
        a_orig.f_stream = nullptr;
        a_orig.f_is_valid = false;
    }

    stream_wrapper::~stream_wrapper()
    {}

    stream_wrapper& stream_wrapper::operator=( stream_wrapper&& a_orig )
    {
        f_stream = a_orig.f_stream;
        a_orig.f_stream = nullptr;
        a_orig.f_is_valid = false;
        return *this;
    }
/*
    monarch3::M3Stream& stream_wrapper::stream()
    {
        if( ! f_is_valid ) throw error() << "Unable to provide the stream; the owning Monarch object must have moved beyond the writing stage";
        return *f_stream;
    }
*/
    /// Write the record contents to the file
    bool stream_wrapper::write_record( bool a_is_new_acq )
    {
        f_stream_mutex.lock();
        bool t_return = f_stream->WriteRecord( a_is_new_acq );
        f_stream_mutex.unlock();
        return t_return;
        //return f_stream->WriteRecord( a_is_new_acq );
    }


/*
    void stream_wrapper::lock()
    {
        f_lock.lock();
        return;
    }
    void stream_wrapper::unlock()
    {
        f_lock.unlock();
        return;
    }
*/
/*
    void stream_wrapper::finish()
    {
        f_stream = nullptr;
        f_is_valid = false;
        return;
    }
*//*
    void stream_wrapper::monarch_stage_change( monarch_stage a_new_stage )
    {
        if( a_new_stage != monarch_stage::writing ) finish();
        return;
    }
*/



    monarch_wrapper::monarch_wrapper( const std::string& a_filename ) :
            f_orig_filename( a_filename ),
            f_filename_base(),
            f_filename_ext(),
            f_file_count( 1 ),
            f_monarch(),
            f_monarch_mutex(),
            f_header_wrap(),
            f_header_mutex(),
            f_stream_wraps(),
            f_run_start_time( std::chrono::steady_clock::now() ),
            f_stage( monarch_stage::initialized )
    {
        std::string::size_type t_ext_pos = a_filename.find_last_of( '.' );
        if( t_ext_pos == std::string::npos )
        {
            f_filename_base = f_orig_filename;
        }
        else
        {
            f_filename_base = f_orig_filename.substr( 0, t_ext_pos );
            f_filename_ext = f_orig_filename.substr( t_ext_pos );
        }

        std::unique_lock< std::mutex > t_monarch_lock( f_monarch_mutex );
        try
        {
            f_monarch.reset( monarch3::Monarch3::OpenForWriting( f_orig_filename ) );
        }
        catch( monarch3::M3Exception& e )
        {
            throw error() << "Unable to open the file <" << f_orig_filename << "\n" <<
                    "Reason: " << e.what();
        }
    }

    monarch_wrapper::~monarch_wrapper()
    {
        f_monarch_mutex.lock();
        try
        {
            if( f_monarch ) f_monarch->FinishWriting();
        }
        catch( monarch3::M3Exception& e )
        {
            LERROR( plog, "Unable to write file on monarch_wrapper deletion: " << e.what() );
        }
        f_monarch_mutex.unlock();
    }

    header_wrap_ptr monarch_wrapper::get_header()
    {
        std::unique_lock< std::mutex > t_monarch_lock( f_monarch_mutex );
        if( f_stage == monarch_stage::initialized )
        {
            try
            {
                set_stage( monarch_stage::preparing );
                header_wrap_ptr t_header_ptr( new header_wrapper( *f_monarch.get(), f_header_mutex ) );
                f_header_wrap = t_header_ptr;
            }
            catch( error& e )
            {
                throw;
            }
        }
        if( f_stage != monarch_stage::preparing ) throw error() << "Invalid monarch stage for getting the header: " << f_stage;

        return f_header_wrap;
    }

    stream_wrap_ptr monarch_wrapper::get_stream( unsigned a_stream_no )
    {
        std::unique_lock< std::mutex > t_monarch_lock( f_monarch_mutex );
        if( f_stage == monarch_stage::preparing )
        {
            f_header_wrap.reset();
            try
            {
                f_monarch->WriteHeader();
                LDEBUG( plog, "Header written for file <" << f_monarch->GetHeader()->GetFilename() << ">" );
            }
            catch( monarch3::M3Exception& e )
            {
                throw error() << e.what();
            }
            set_stage( monarch_stage::writing );
        }
        if( f_stage != monarch_stage::writing ) throw error() << "Invalid monarch stage for getting a stream: " << f_stage;

        if( f_stream_wraps.find( a_stream_no ) == f_stream_wraps.end() )
        {
            try
            {
                stream_wrap_ptr t_stream_ptr( new stream_wrapper( *f_monarch.get(), a_stream_no ) );
                f_stream_wraps[ a_stream_no ] = t_stream_ptr;
            }
            catch( error& e )
            {
                throw;
            }
        }
        return f_stream_wraps[ a_stream_no ];
    }

    void monarch_wrapper::finish_stream( unsigned a_stream_no, bool a_finish_if_streams_done )
    {
        std::unique_lock< std::mutex > t_monarch_lock( f_monarch_mutex );
        if( f_stage != monarch_stage::writing )
        {
            throw error() << "Monarch must be in the writing stage to finish a stream";
        }

        auto t_stream_it = f_stream_wraps.find( a_stream_no );
        if( t_stream_it == f_stream_wraps.end() )
        {
            throw error() << "Stream number <" << a_stream_no << "> was not found";
        }
        LDEBUG( plog, "Finished stream <" << a_stream_no << ">" );
        f_stream_wraps.erase( t_stream_it );

        if( a_finish_if_streams_done && f_stream_wraps.empty() )
        {
            try
            {
                LDEBUG( plog, "All streams complete; automatically finishing file <" << f_monarch->GetHeader()->GetFilename() << ">" );
                finish_file_nolock();
            }
            catch( error& e )
            {
                throw;
            }
        }

        return;
    }

    void monarch_wrapper::finish_file()
    {
        std::unique_lock< std::mutex > t_monarch_lock( f_monarch_mutex );
        try
        {
            finish_file_nolock();
        }
        catch( error& e )
        {
            throw;
        }
        return;
    }

    void monarch_wrapper::finish_file_nolock()
    {
        std::string t_filename( f_monarch->GetHeader()->GetFilename() );
        if( f_stage == monarch_stage::preparing )
        {
            f_header_wrap.reset();
            try
            {
                f_monarch->WriteHeader();
                LDEBUG( plog, "Header written for file <" << t_filename << ">" );
            }
            catch( monarch3::M3Exception& e )
            {
                throw error() << e.what();
            }
        }
        else if( f_stage == monarch_stage::writing )
        {
            if( ! f_stream_wraps.empty() )
            {
                throw error() << "Streams have not all been finished";
            }
        }
        LINFO( plog, "Finished writing file <" << t_filename << ">" );
        f_monarch->FinishWriting();
        set_stage( monarch_stage::finished );
        f_monarch.reset();
        butterfly_house::get_instance()->remove_file( f_orig_filename );
        return;
    }

    void monarch_wrapper::switch_to_new_file()
    {
        if( f_stage != monarch_stage::writing ) throw error() << "Invalid monarch stage for starting a new file: " << f_stage;

        std::unique_lock< std::mutex > t_monarch_lock( f_monarch_mutex );

        set_stage( monarch_stage::initialized );

        // create the new filename
        std::stringstream t_count_stream;
        t_count_stream << f_file_count;
        std::string t_new_filename = f_filename_base + '_' + t_count_stream.str() + f_filename_ext;

        // open the new file
        std::unique_ptr< monarch3::Monarch3 > t_new_monarch;
        try
        {
            t_new_monarch.reset( monarch3::Monarch3::OpenForWriting( t_new_filename ) );
        }
        catch( monarch3::M3Exception& e )
        {
            throw error() << "Unable to open the file <" << t_new_filename << "\n" <<
                    "Reason: " << e.what();
        }

        set_stage( monarch_stage::preparing );

        // copy info into the new header
        monarch3::M3Header* t_new_header = t_new_monarch->GetHeader();
        t_new_header->CopyBasicInfo( *(f_monarch->GetHeader()) );
        t_new_header->SetFilename( t_new_filename );
        t_new_header->SetDescription( t_new_header->GetDescription() + "\nContinuation of file " + f_orig_filename );

        // for each stream, lock stream pointer, create new stream in new file, and remove the reference to the old stream (the stream itself is deleted in Monarch3::FinishWriting())
        std::vector< monarch3::M3StreamHeader >* t_old_stream_headers = &f_monarch->GetHeader()->GetStreamHeaders();
        for( std::map< unsigned, stream_wrap_ptr >::iterator t_stream_it = f_stream_wraps.begin(); t_stream_it != f_stream_wraps.end(); ++t_stream_it )
        {
            t_stream_it->second->lock();
            monarch3::M3StreamHeader* t_old_stream_header = &t_old_stream_headers->operator[]( t_stream_it->first );
            if( t_old_stream_header->GetNChannels() > 1 )
            {
                t_new_header->AddStream( t_old_stream_header->GetSource(), t_old_stream_header->GetNChannels(), t_old_stream_header->GetChannelFormat(),
                        t_old_stream_header->GetAcquisitionRate(), t_old_stream_header->GetRecordSize(), t_old_stream_header->GetSampleSize(),
                        t_old_stream_header->GetDataTypeSize(), t_old_stream_header->GetDataFormat(),
                        t_old_stream_header->GetBitDepth(), t_old_stream_header->GetBitAlignment() );
            }
            else
            {
                t_new_header->AddStream( t_old_stream_header->GetSource(),
                        t_old_stream_header->GetAcquisitionRate(), t_old_stream_header->GetRecordSize(), t_old_stream_header->GetSampleSize(),
                        t_old_stream_header->GetDataTypeSize(), t_old_stream_header->GetDataFormat(),
                        t_old_stream_header->GetBitDepth(), t_old_stream_header->GetBitAlignment() );
            }
            t_stream_it->second->f_stream = nullptr;
        }

        // finish the old file
        f_monarch->FinishWriting();
        f_monarch.reset();

        // write the new header
        t_new_monarch->WriteHeader();

        // switch out the monarch pointers, f_monarch <--> t_new_monarch
        f_monarch.swap( t_new_monarch );
        set_stage( monarch_stage::writing );

        // put references to new streams in each stream pointer, and then unlock the streams
        for( std::map< unsigned, stream_wrap_ptr >::iterator t_stream_it = f_stream_wraps.begin(); t_stream_it != f_stream_wraps.end(); ++t_stream_it )
        {
            t_stream_it->second->f_stream = f_monarch->GetStream( t_stream_it->first );
            if( t_stream_it->second->f_stream == nullptr )
            {
                throw error() << "Stream <" << t_stream_it->first << "> was invalid";
            }
            t_stream_it->second->unlock();
        }

        return;
    }

    void monarch_wrapper::set_stage( monarch_stage a_stage )
    {
        LDEBUG( plog, "Setting monarch stage to <" << a_stage << ">" );
        f_stage = a_stage;
        //if( f_header_wrap ) f_header_wrap->monarch_stage_change( a_stage );
        /*
        for( std::vector< stream_wrap_ptr >::iterator t_it = f_stream_wraps.begin(); t_it != f_stream_wraps.end(); ++t_it )
        {
            if( *t_it )
            {
                (*t_it)->monarch_stage_change( a_stage );
            }
        }
        */
        return;
    }

} /* namespace psyllid */
