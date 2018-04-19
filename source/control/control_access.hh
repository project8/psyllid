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
            control_access();
            virtual ~control_access();

            static void set_daq_control( std::weak_ptr< daq_control > a_daq_control );

        protected:
            static std::weak_ptr< daq_control > f_daq_control;

            std::shared_ptr< daq_control > use_daq_control() {return control_access::f_daq_control.lock();}

            bool daq_control_expired() {return control_access::f_daq_control.expired();}
    };

} /* namespace psyllid */

#endif /* PSYLLID_CONTROL_ACCESS_HH_ */
