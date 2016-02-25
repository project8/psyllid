/*
 * monarch3_wrap.hh
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 *
 *  Monarch stages:
 *    - initialized
 *    - preparing
 *    - writing
 *    - finished
 *
 *  Thread safety
 *    Thread-safe operations:
 *      - Initializing an egg file
 *      - Accessing the Monarch header (can only be done by one thread at a time)
 *      - Writing data to a file (handled by HDF5's internal thread safety)
 *      - Finishing an egg file
 *
 *    Non-thread-safe operations:
 *      - Stream function calls are not thread-safe other than HDF5's internal thread safety.
 *        It is highly (highly highly) recommended that you only access a given stream from one thread.
 */

#ifndef PSYLLID_MONARCH3_WRAP_HH_
#define PSYLLID_MONARCH3_WRAP_HH_

#include "M3Monarch.hh"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>

namespace psyllid
{

    enum class monarch_stage
    {
        initialized = 0,
        preparing = 1,
        writing = 2,
        finished = 3
    };
    uint32_t to_uint( monarch_stage a_stage );
    monarch_stage to_stage( uint32_t a_stage_uint );
    std::ostream& operator<<( std::ostream& a_os, monarch_stage a_stage );


    class header_wrapper;
    typedef std::shared_ptr< header_wrapper > header_wrap_ptr;

    class stream_wrapper;
    typedef std::shared_ptr< stream_wrapper > stream_wrap_ptr;

    typedef std::chrono::time_point< std::chrono::steady_clock, std::chrono::nanoseconds > monarch_time_point_t;

    class monarch_wrapper
    {
        public:
            monarch_wrapper( const std::string& a_filename );
            ~monarch_wrapper();

            header_wrap_ptr get_header();

            stream_wrap_ptr get_stream( unsigned a_stream_no );


            void finish_stream( unsigned a_stream_no, bool a_finish_if_streams_done = false );
            void finish_file();

            monarch_time_point_t get_run_start_time() const;

            void set_stage( monarch_stage a_stage );

        private:
            void finish_file_nolock();

            monarch_wrapper( const monarch_wrapper& ) = delete;
            monarch_wrapper& operator=( const monarch_wrapper& ) = delete;

            std::unique_ptr< monarch3::Monarch3 > f_monarch;
            mutable std::mutex f_monarch_mutex;

            header_wrap_ptr f_header_wrap;
            mutable std::mutex f_header_mutex;

            std::map< unsigned, stream_wrap_ptr > f_stream_wraps;

            monarch_time_point_t f_run_start_time;

            monarch_stage f_stage;

    };

    typedef std::shared_ptr< monarch_wrapper > monarch_wrap_ptr;


    class header_wrapper
    {
        public:
            header_wrapper( monarch3::Monarch3& a_monarch, std::mutex& a_mutex );
            header_wrapper( header_wrapper&& a_orig );
            ~header_wrapper();

            header_wrapper& operator=( header_wrapper&& a_orig );

            // Will throw psyllid::error if the header object is not valid.
            monarch3::M3Header& header();

            bool global_setup_done() const;
            void global_setup_done( bool a_flag );

        private:
            header_wrapper( const header_wrapper& ) = delete;
            header_wrapper& operator=( const header_wrapper& ) = delete;

            //friend class butterfly_house;

            //void monarch_stage_change( monarch_stage a_new_stage );

            monarch3::M3Header* f_header;
            std::unique_lock< std::mutex > f_lock;

            bool f_global_setup_done;
    };


    class stream_wrapper
    {
        public:
            stream_wrapper( monarch3::Monarch3&, unsigned a_stream_no );
            stream_wrapper( stream_wrapper&& a_orig );
            ~stream_wrapper();

            stream_wrapper& operator=( stream_wrapper&& a_orig );

            bool is_valid() const;

            // Will throw psyllid::error if the stream object is not valid.
            // This includes the situation where the monarch stage is anything other than writing.
            //monarch3::M3Stream& stream();

            /// Get the pointer to the stream record
            monarch3::M3Record* get_stream_record();
            /// Get the pointer to a particular channel record
            monarch3::M3Record* get_channel_record( unsigned a_chan_no );

            /// Write the record contents to the file
            bool write_record( bool a_is_new_acq );

            //void lock();
            //void unlock();

            /// Complete use of this stream; does not write the file to disk
            //void finish();

        private:
            stream_wrapper( const stream_wrapper& ) = delete;
            stream_wrapper& operator=( const stream_wrapper& ) = delete;

            //friend class monarch_wrapper;

            //void monarch_stage_change( monarch_stage a_new_stage );

            monarch3::M3Stream* f_stream;
            bool f_is_valid;
            //std::mutex f_stream_mutex;
            //std::unique_lock< std::mutex > f_lock;
    };

    inline monarch_time_point_t monarch_wrapper::get_run_start_time() const
    {
        return f_run_start_time;
    }

    inline bool header_wrapper::global_setup_done() const
    {
        return f_global_setup_done;
    }

    inline void header_wrapper::global_setup_done( bool a_flag )
    {
        f_global_setup_done = a_flag;
        return;
    }


    inline bool stream_wrapper::is_valid() const
    {
        return f_is_valid;
    }


} /* namespace psyllid */

#endif /* PSYLLID_MONARCH3_WRAP_HH_ */
