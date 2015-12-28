/*
 * freq_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_FREQ_DATA_HH_
#define DATA_FREQ_DATA_HH_

#include "macros.hh"
#include "types.hh"

namespace psyllid
{

    class freq_data
    {
        public:
            freq_data();
            virtual ~freq_data();

        public:
            accessible( midge::count_t, id );
            referrable( midge::real_t, freq_value );
    };

} /* namespace psyllid */

#endif /* DATA_FREQ_DATA_HH_ */
