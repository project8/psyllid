/*
 * trigger_flag.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_TRIGGER_FLAG_HH_
#define DATA_TRIGGER_FLAG_HH_

#include "member_variables.hh"

#include <cstdint>

namespace psyllid
{

    class trigger_flag
    {
        public:
            trigger_flag();
            virtual ~trigger_flag();

        public:
            mv_accessible( bool, flag );
            mv_accessible( uint64_t, id );
            mv_accessible( unsigned, threshold);
    };

} /* namespace psyllid */

#endif /* DATA_TRIGGER_FLAG_HH_ */
