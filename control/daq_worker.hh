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

            void execute( std::shared_ptr< node_manager > a_node_mgr, std::exception_ptr a_ex_ptr );

        private:
            virtual void do_cancellation();

            midge_package f_midge_pkg;
    };

} /* namespace psyllid */

#endif /* PSYLLID_DAQ_WORKER_HH_ */
