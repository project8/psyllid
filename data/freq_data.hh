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

#include <vector>
using std::vector;

#include <memory>
using std::unique_ptr;

namespace psyllid
{

    class freq_data
    {
        public:
            freq_data();
            virtual ~freq_data();

        public:
            accessible( midge::count_t, id );
            referrable( unique_ptr< vector< midge::real_t > >, array );
    };

} /* namespace psyllid */

#endif /* DATA_FREQ_DATA_HH_ */
