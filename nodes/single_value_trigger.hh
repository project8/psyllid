/*
 * single_value_trigger.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef NODES_SINGLE_VALUE_TRIGGER_HH_
#define NODES_SINGLE_VALUE_TRIGGER_HH_

#include "freq_data.hh"
#include "trigger_flag.hh"

#include "transformer.hh"

using namespace midge;

namespace psyllid
{

    class single_value_trigger :
            public _transformer< single_value_trigger, typelist_1( freq_data ), typelist_1( trigger_flag ) >
    {
        public:
            single_value_trigger();
            virtual ~single_value_trigger();

        public:
            accessible( count_t, length );
            accessible( real_t, threshold );

        public:
            void initialize();
            void execute();
            void finalize();

    };

} /* namespace psyllid */

#endif /* NODES_SINGLE_VALUE_TRIGGER_HH_ */
