/*
 * terminator_freq_data.hh
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_TERMINATOR_FREQ_DATA_HH_
#define PSYLLID_TERMINATOR_FREQ_DATA_HH_

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


} /* namespace psyllid */

#endif /* PSYLLID_TERMINATOR_FREQ_DATA_HH_ */
