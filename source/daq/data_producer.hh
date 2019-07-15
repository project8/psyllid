/*
 * data_producer.hh
 *
 *  Created on: May 31, 2019
 *      Author: N.S. Oblath
 */

#ifndef PSYLLID_DAQ_DATA_PRODUCER_HH_
#define PSYLLID_DAQ_DATA_PRODUCER_HH_

#include "memory_block.hh"
#include "node_builder.hh"

#include "producer.hh"

#include "roach_packet.hh"

namespace psyllid
{

    /*!
     @class data_producer
     @author N. S. Oblath

     @brief A producer to use for debugging: continously outputss identical blank data

     @details

     The data are all output as `memory_block` objects.
     However the underlying data that is output is time_data.

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "data-producer"

     Available configuration values:
     - "length": uint -- The size of the output data buffer
     - "data-size": uint -- The size of the output data objects

     Output Stream:
     - 0: memory_block

    */
    class data_producer : public midge::_producer< midge::type_list< memory_block > >
    {
        public:
            data_producer();
            virtual ~data_producer();

            mv_accessible( uint64_t, length );
            mv_accessible( uint32_t, data_size );

            mv_referrable( roach_packet_data, master_packet );

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            void initialize_block( memory_block* a_block );

    };

    class data_producer_binding : public _node_binding< data_producer, data_producer_binding >
    {
        public:
            data_producer_binding();
            virtual ~data_producer_binding();

        private:
            virtual void do_apply_config( data_producer* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const data_producer* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace dripline */

#endif /* PSYLLID_DAQ_DATA_PRODUCER_HH_ */
