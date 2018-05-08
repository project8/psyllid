/*
 * batch_executor.hh
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

#ifndef PSYLLID_BATCH_EXECUTOR_HH_
#define PSYLLID_BATCH_EXECUTOR_HH_

// scarab includes
#include "cancelable.hh"
#include "param.hh"

// dripline
#include "message.hh"

namespace psyllid
{
    /*!
     @class batch_executor
     @author B. H. LaRoque

     @brief A class sequentially execute a list of actions, equivalent to a sequence of dripline requests

     @details

    batch_executor is a control component used to submit a pre-defined sequence of requests upon startup and in a non-interactive way.
    This allows submission of any request which would normally be sent via a dripline request.
    It can also supports (currently one) custom command (type = wait-for), which will poll the provided rks and wait for the return payload to not indicate a daq_status of running.

    The actions taken are defined in the top-level param_node of psyllid's config file named "batch-actions", which must be of type array.
    Each element of the array is expected to be another param_node and have the following keys:
    - type (str): valid dripline::msg_op or "wait-for"
    - rks (str): full routing-key-specifier for the desired action
    - payload (param_node): request message payload content for the action
    - sleep-for (int) [optional]: time in milliseconds for which the thread will sleep after receiving a reply on the specified request. Note i) that each request blocks until a reply is generated, but triggered actions may or may not be ongoing; ii) the "wait-for" action type sleeps this much time after *each* poll.

    */

    // forward declarations
    class request_receiver;

    struct action_info
    {
        bool f_is_custom_action;
        dripline::request_ptr_t f_request_ptr;
        unsigned f_sleep_duration_ms;
    };

    // local content
    class batch_executor : public scarab::cancelable
    {
        public:
            batch_executor();
            batch_executor( const scarab::param_node& a_master_config, std::shared_ptr<psyllid::request_receiver> a_request_receiver );
            virtual ~batch_executor();

            void execute();

        private:
            scarab::param_array f_actions_array;
            std::shared_ptr<request_receiver> f_request_receiver;

            virtual void do_cancellation();
            static action_info parse_action( const scarab::param_node* a_action );

    };

} /* namespace psyllid */

#endif /*PSYLLID_BATCH_EXECUTOR_HH_*/
