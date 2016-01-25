/*
 * daq_worker.hh
 *
 *  Created on: Jan 24, 2016
 *      Author: nsoblath
 */

#ifndef CONTROL_DAQ_WORKER_HH_
#define CONTROL_DAQ_WORKER_HH_

#include "cancelable.hh"

namespace psyllid
{

    class daq_worker : public midge::cancelable
    {
        public:
            daq_worker();
            virtual ~daq_worker();

            void execute();

        private:
            virtual void do_cancellation();
    };

} /* namespace psyllid */

#endif /* CONTROL_DAQ_WORKER_HH_ */
