/*
 * batch_executor.hh
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

#ifndef PSYLLID_BATCH_EXECUTOR_HH_
#define PSYLLID_BATCH_EXECUTOR_HH_

namespace psyllid
{
    /*!
     @class batch_executor
     @author B. H. LaRoque

     @brief A class sequentially execute a list of actions, equivalent to a sequence of dripline requests

     @details

     ... tbd ...

    */

    class batch_executor
    {
        public:
            batch_executor();
            virtual ~batch_executor();
    };

} /* namespace psyllid */

#endif /*PSYLLID_BATCH_EXECUTOR_HH_*/
