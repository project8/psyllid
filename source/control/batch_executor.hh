/*
 * batch_executor.hh
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

#ifndef PSYLLID_BATCH_EXECUTOR_HH_
#define PSYLLID_BATCH_EXECUTOR_HH_

#include "param.hh"

namespace psyllid
{
    /*!
     @class batch_executor
     @author B. H. LaRoque

     @brief A class sequentially execute a list of actions, equivalent to a sequence of dripline requests

     @details

     ... tbd ...

    */

    //TODO should be scarab::cancelable?
    class batch_executor
    {
        public:
            batch_executor();
            batch_executor( const scarab::param_node& a_master_config );
            virtual ~batch_executor();

            void execute();
    };

} /* namespace psyllid */

#endif /*PSYLLID_BATCH_EXECUTOR_HH_*/
