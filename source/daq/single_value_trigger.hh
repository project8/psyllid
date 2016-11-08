/*
 * single_value_trigger.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_SINGLE_VALUE_TRIGGER_HH_
#define PSYLLID_SINGLE_VALUE_TRIGGER_HH_

#include "freq_data.hh"
#include "trigger_flag.hh"

#include "transformer.hh"

namespace psyllid
{

    class single_value_trigger :
            public midge::_transformer< single_value_trigger, typelist_1( freq_data ), typelist_1( trigger_flag ) >
    {
        public:
            single_value_trigger();
            virtual ~single_value_trigger();

        public:
            accessible( uint64_t, length );
            accessible( double, threshold );

        public:
            void initialize();
            void execute( midge::diptera* a_midge = nullptr );
            void finalize();

    };

} /* namespace psyllid */

#endif /* PSYLLID_SINGLE_VALUE_TRIGGER_HH_ */
