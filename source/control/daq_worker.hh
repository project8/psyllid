/*
 * daq_worker.hh
 *
 *  Created on: Jan 24, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_DAQ_WORKER_HH_
#define PSYLLID_DAQ_WORKER_HH_

#include "node_manager.hh" // for midge_package

#include "cancelable.hh"

#include <functional>
#include <memory>

namespace midge
{
    class diptera;
}

namespace psyllid
{
    class node_manager;

    class daq_worker : public midge::cancelable
    {
        public:
            daq_worker();
            virtual ~daq_worker();

            void execute( std::shared_ptr< node_manager > a_node_mgr, std::exception_ptr a_ex_ptr, std::function< void() > a_notifier );

            void start_run();
            void stop_run();

        private:
            virtual void do_cancellation();

            midge_package f_midge_pkg;

            std::function< void() > f_stop_notifier;
    };

} /* namespace psyllid */

#endif /* PSYLLID_DAQ_WORKER_HH_ */
