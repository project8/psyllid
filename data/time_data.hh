/*
 * time_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_TIME_DATA_HH_
#define DATA_TIME_DATA_HH_

#include "macros.hh"
#include "types.hh"

namespace psyllid
{

    class time_data
    {
        public:
            time_data();
            virtual ~time_data();

        public:
            accessible( midge::count_t, id );
            referrable( int8_t, time_value );
    };

} /* namespace psyllid */

#endif /* DATA_TIME_DATA_HH_ */
