/*
 * egg3_reader.hh
 *
 *  Created on: Dec 14, 2017
 *      Author: laroque
 */

#ifndef PSYLLID_EGG3_READER_HH_
#define PSYLLID_EGG3_READER_HH_

#include "memory_block.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "producer.hh"
#include "M3Monarch.hh"

namespace psyllid
{

    /*!
     @class egg3_reader
     @author B. H. LaRoque

     @brief A producer to read time-domain slices from an egg file and place them in time data buffers.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "egg3-reader"

     Available configuration values:
     - "path": string -- resolvable path to the egg file from which to read data

     Output Streams:
     - 0: time_data
    */
    class egg3_reader : public midge::_producer< egg3_reader, typelist_1( time_data ) >
    {
        public:
            egg3_reader();
            virtual ~egg3_reader();

        public:
            mv_accessible( const monarch3::Monarch3*, egg );
            mv_accessible( std::string, egg_path );
            mv_accessible( uint64_t, length );

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            void cleanup_file();

    };

    class egg3_reader_binding : public _node_binding< egg3_reader, egg3_reader_binding >
    {
        public:
            egg3_reader_binding();
            virtual ~egg3_reader_binding();
        private:
            virtual void do_apply_config( egg3_reader* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const egg3_reader* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_EGG3_READER_HH_ */
