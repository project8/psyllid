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

#include "producer.hh"
#include "control_access.hh"

namespace monarch3
{
    class Monarch3;
    class M3Stream;
    class M3Record;
}

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
     - "egg-path": string -- resolvable path to the egg file from which to read data
     - "read-n-records": int -- number of records to read from file when executing, 0 means until end of file
     - "repeat-egg": bool -- indicates if reaching the end of input file should end the reading (false) or loop back to the start of the file (true); default is false

     Output Streams:
     - 0: time_data
    */

    // forward declarations
    class time_data;

    // egg3_reader
    class egg3_reader : public midge::_producer< egg3_reader, typelist_1( time_data ) >, public control_access
    {
        public:
            egg3_reader();
            virtual ~egg3_reader();

        public:
            mv_accessible( const monarch3::Monarch3*, egg );
            mv_accessible( std::string, egg_path );
            mv_accessible( uint64_t, read_n_records );
            mv_accessible( bool, repeat_egg );
            mv_accessible( uint64_t, length );
            mv_accessible( bool, start_paused );

        private:
            bool f_paused;
            uint32_t f_record_length;
            uint64_t f_pkt_id_offset;

       public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            bool read_slice( time_data* t_data, const monarch3::M3Stream* t_stream, const monarch3::M3Record* t_record);
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
