/*
 * egg3_reader.hh
 *
 *  Created on: Dec 14, 2017
 *      Author: laroque
 */

#ifndef PSYLLID_EGG3_READER_HH_
#define PSYLLID_EGG3_READER_HH_

#include "time_data.hh"
//#include "node_builder.hh"

#include "producer.hh"
//#include "shared_cancel.hh"

//namespace scarab
//{
//    class param_node;
//}

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
            mv_accessible( uint64_t, length );
//            mv_accessible( uint32_t, max_packet_size );
//            mv_accessible( uint32_t, port );
//            mv_referrable( std::string, ip );
//            mv_accessible( unsigned, timeout_sec );  /// Timeout in seconds for waiting on socket recv function
//
        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();
//
//        private:
//            void cleanup_socket();
//
//            int f_socket;
//            sockaddr_in* f_address;
//
//        protected:
//            int f_last_errno;
//
    };

    class egg3_reader_binding: public _node_binding< egg3_reader, egg3_reader_binding >
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
