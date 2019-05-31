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

namespace psyllid
{

    class data_producer : public midge::_producer< midge::type_list< memory_block > >
    {
        public:
            data_producer();
            virtual ~data_producer();

            mv_accessible( uint64_t, length );
            mv_accessible( uint32_t, max_packet_size );

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

};

} /* namespace dripline */

#endif /* PSYLLID_DAQ_DATA_PRODUCER_HH_ */
