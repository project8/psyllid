/*
 * terminator_freq_data.hh
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_TERMINATOR_FREQ_DATA_HH_
#define PSYLLID_TERMINATOR_FREQ_DATA_HH_

#include "node_builder.hh"

#include "consumer.hh"

#include "freq_data.hh"

namespace psyllid
{

    class terminator_freq_data :
            public midge::_consumer< terminator_freq_data, typelist_1( freq_data ) >
    {
        public:
            terminator_freq_data();
            virtual ~terminator_freq_data();

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

    };

    class terminator_freq_data_builder : public _node_builder< terminator_freq_data >
    {
        public:
            terminator_freq_data_builder();
            virtual ~terminator_freq_data_builder();

        private:
            virtual void apply_config( terminator_freq_data* a_node, const scarab::param_node& a_config );
    };


} /* namespace psyllid */

#endif /* PSYLLID_TERMINATOR_FREQ_DATA_HH_ */
