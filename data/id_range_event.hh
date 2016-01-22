/*
 * id_range_event.hh
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#ifndef DATA_ID_RANGE_EVENT_HH_
#define DATA_ID_RANGE_EVENT_HH_

#include "member_variables.hh"

namespace psyllid
{

    class id_range_event
    {
        public:
            id_range_event();
            virtual ~id_range_event();

        public:
            mv_accessible( uint64_t, start_id );
            mv_accessible( uint64_t, end_id );
    };

} /* namespace psyllid */

#endif /* DATA_ID_RANGE_EVENT_HH_ */
