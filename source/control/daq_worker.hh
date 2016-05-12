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

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

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

            void start_run( unsigned a_duration = 0 );
            void stop_run();

        private:
            virtual void do_cancellation();

            void do_run( unsigned a_duration );

            midge_package f_midge_pkg;

            std::atomic< bool > f_run_in_progress;
            std::condition_variable f_run_stopper;
            std::mutex f_run_stop_mutex;

            std::function< void() > f_stop_notifier;
    };

} /* namespace psyllid */

#endif /* PSYLLID_DAQ_WORKER_HH_ */
