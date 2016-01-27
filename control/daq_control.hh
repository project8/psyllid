/*
 * daq_control.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_DAQ_CONTROL_HH_
#define PSYLLID_DAQ_CONTROL_HH_

#include "member_variables.hh"

#include "psyllid_error.hh"

#include "cancelable.hh"

#include <atomic>
#include <memory>

namespace psyllid
{
    class daq_worker;
    class node_manager;

    class daq_control : public midge::cancelable
    {
        public:
            class run_error : public error
            {
                public:
                    run_error() {}
                    virtual ~run_error() {}
            };

        public:
            daq_control( std::shared_ptr< node_manager > a_mgr );
            virtual ~daq_control();

            /// Start a run
            /// Can throw daq_control::run_error; daq_control object will still be usable
            /// Can throw psyllid::error; daq_control object will NOT be usable
            void do_run();

            mv_atomic( bool, is_running );

        private:
            void do_cancellation();

            std::shared_ptr< node_manager > f_node_manager;

            std::shared_ptr< daq_worker > f_daq_worker;
            std::mutex f_worker_mutex;
    };

} /* namespace psyllid */

#endif /* PSYLLID_DAQ_CONTROL_HH_ */
