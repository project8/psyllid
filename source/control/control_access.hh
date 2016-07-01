/*
 * control_access.hh
 *
 *  Created on: Feb 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_CONTROL_ACCESS_HH_
#define PSYLLID_CONTROL_ACCESS_HH_

#include <memory>

namespace psyllid
{
    class daq_control;

    class control_access
    {
        public:
            control_access( std::weak_ptr< daq_control > a_daq_control = std::weak_ptr< daq_control >() );
            virtual ~control_access();

            void set_daq_control( std::weak_ptr< daq_control > a_daq_control );

        protected:
            std::weak_ptr< daq_control > f_daq_control;
    };

} /* namespace psyllid */

#endif /* PSYLLID_CONTROL_ACCESS_HH_ */
