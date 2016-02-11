/*
 * butterfly_house.hh
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 *
 *  Monarch stages:
 *    - initialized
 *    - preparing
 *    - writing
 *    - finished
 */

#ifndef PSYLLID_BUTTERFLY_HOUSE_HH_
#define PSYLLID_BUTTERFLY_HOUSE_HH_

#include "M3Monarch.hh"

#include "singleton.hh"

#include <map>
#include <memory>

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

    class butterfly_house;

    class header_wrapper;
    typedef std::shared_ptr< header_wrapper > header_wrap_ptr;

    class stream_wrapper;
    typedef std::shared_ptr< stream_wrapper > stream_wrap_ptr;



    class butterfly_house : public scarab::singleton< butterfly_house >
    {


            /// Creates the Monarch3 object for the given filename
            /// Sets the Monarch stage to "initializing"
            void declare_file( const std::string& a_filename );

            /// Requires that the Monarch stage be "initializing" or "preparing"
            /// If it's in the former, sets the stage to "preparing"
            /// Throws psyllid::error if anything fails
            header_wrap_ptr get_header( const std::string& a_filename ) const;

            /// Requires that the Monarch stage be "preparing" or "writing"
            /// If it's the former, sets the stage to "writing" and writes the Monarch header to the file
            /// Throws psyllid::error if anything fails
            stream_wrap_ptr get_stream( const std::string& a_filename, unsigned a_stream ) const;

            /// Closes the file
            /// Sets the Monarch stage to "finished"
            void write_file( const std::string& a_filename );

        private:
            class monarch_wrapper
            {
                public:
                    monarch_wrapper( const std::string& a_filename );
                    ~monarch_wrapper();

                    header_wrap_ptr get_header();

                    stream_wrap_ptr get_stream( unsigned a_stream_no );

                    void finish_file();

                    void set_stage( monarch_stage a_stage );

                private:
                    monarch_wrapper( const monarch_wrapper& ) = delete;
                    monarch_wrapper& operator=( const monarch_wrapper& ) = delete;

                    std::unique_ptr< monarch3::Monarch3 > f_monarch;
                    header_wrap_ptr f_header_wrap;
                    std::vector< stream_wrap_ptr > f_stream_wraps;
                    monarch_stage f_stage;

            };
            typedef std::unique_ptr< monarch_wrapper > monarch_wrap_ptr;


            typedef std::map< std::string, monarch_wrap_ptr > bf_map;
            typedef bf_map::const_iterator bf_cit_t;
            typedef bf_map::iterator bf_it_t;
            typedef bf_map::value_type bf_value_t;

            bf_map f_butterflies;


        private:
            friend class scarab::singleton< butterfly_house >;
            friend class scarab::destroyer< butterfly_house >;

            butterfly_house();
            virtual ~butterfly_house();

    };


    class header_wrapper
    {
        public:
            header_wrapper( monarch3::Monarch3& a_monarch );
            ~header_wrapper();

            monarch3::M3Header& header();

        private:
            header_wrapper( const header_wrapper& ) = delete;
            header_wrapper& operator=( const header_wrapper& ) = delete;

            friend class butterfly_house;

            void monarch_stage_change( monarch_stage a_new_stage );

            monarch3::M3Header* f_header;
    };


    class stream_wrapper
    {
        public:
            stream_wrapper( monarch3::Monarch3&, unsigned a_stream_no );
            ~stream_wrapper();

            monarch3::M3Stream& stream();

        private:
            stream_wrapper( const stream_wrapper& ) = delete;
            stream_wrapper& operator=( const stream_wrapper& ) = delete;

            friend class butterfly_house;

            void monarch_stage_change( monarch_stage a_new_stage );

            monarch3::M3Stream* f_stream;
    };



} /* namespace psyllid */

#endif /* PSYLLID_BUTTERFLY_HOUSE_HH_ */
