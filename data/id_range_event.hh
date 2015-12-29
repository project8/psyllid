/*
 * id_range_event.hh
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#ifndef DATA_ID_RANGE_EVENT_HH_
#define DATA_ID_RANGE_EVENT_HH_

#include "macros.hh"
#include "types.hh"

namespace psyllid
{

    class id_range_event
    {
        public:
            id_range_event();
            virtual ~id_range_event();

        public:
            accessible( midge::count_t, start_id );
            accessible( midge::count_t, end_id );
    };

} /* namespace psyllid */

#endif /* DATA_ID_RANGE_EVENT_HH_ */
