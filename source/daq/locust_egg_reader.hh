/*
 * locust_egg_reader.hh
 *
 *  Created on: Dec 13, 2024
 *      Author: pkolbeck
 */

#ifndef PSYLLID_LOCUST_EGG_READER_HH_
#define PSYLLID_LOCUST_EGG_READER_HH_

#include "producer.hh"

#include "control_access.hh"
#include "memory_block.hh"
#include "node_builder.hh"

namespace monarch3
{
    class Monarch3;
    class M3Stream;
    class M3Record;
}

namespace psyllid
{
    /*!
     @class locust_egg_reader
     @author P. T. Kolbeck

     @brief A producer to read time-domain slices from a locust-style egg file and place them in time data buffers.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "locust-egg-reader" TODO: CHANGE NODE NAME, NEED TO ADD TO OTHER FILES

     Available configuration values: TODO: CHANGE PARAMETERS. WHAT PARAMETERS ARE USEFUL? 
     - "egg-path": string -- resolvable path to the egg file from which to read data
     - "read-n-records": int -- number of records to read from file when executing, 0 means until end of file
     - "repeat-egg": bool -- indicates if reaching the end of input file should end the reading (false) or loop back to the start of the file (true); default is false

     Output Streams:
     - 0: time_data
    */

    // forward declarations
    class time_data;

    // locust_egg_reader
    class locust_egg_reader : public midge::_producer< midge::type_list<time_data> >, public sandfly::control_access
    {
        public:
            locust_egg_reader();
            virtual ~locust_egg_reader();

        public:
            mv_accessible( const monarch3::Monarch3*, egg );
            mv_accessible( std::string, egg_path );
            mv_accessible( uint64_t, read_n_records );
            mv_accessible( bool, repeat_egg );
            mv_accessible( uint64_t, length );
            mv_accessible( bool, start_paused );
            mv_accessible( uint64_t, slice_length);

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

    class locust_egg_reader_binding : public sandfly::_node_binding< locust_egg_reader, locust_egg_reader_binding >
    {
        public:
            locust_egg_reader_binding();
            virtual ~locust_egg_reader_binding();
        private:
            virtual void do_apply_config( locust_egg_reader* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const locust_egg_reader* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_LOCUST_EGG_READER_HH_ */
