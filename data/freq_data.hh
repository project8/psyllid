/*
 * freq_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_FREQ_DATA_HH_
#define DATA_FREQ_DATA_HH_

#include "member_variables.hh"

#include <vector>

#include <memory>

namespace psyllid
{

    class freq_data
    {
        public:
            freq_data();
            virtual ~freq_data();

        public:
            mv_accessible( uint64_t, id );
            mv_referrable( std::unique_ptr< std::vector< double > >, array );
    };

} /* namespace psyllid */

#endif /* DATA_FREQ_DATA_HH_ */
