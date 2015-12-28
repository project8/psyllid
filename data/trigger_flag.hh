/*
 * trigger_flag.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_TRIGGER_FLAG_HH_
#define DATA_TRIGGER_FLAG_HH_

#include "macros.hh"
#include "types.hh"

namespace psyllid
{

    class trigger_flag
    {
        public:
            trigger_flag();
            virtual ~trigger_flag();

        public:
            accessible( bool, flag );
            accessible( midge::count_t, id );
    };

} /* namespace psyllid */

#endif /* DATA_TRIGGER_FLAG_HH_ */
