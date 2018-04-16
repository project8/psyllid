/*
 * batch_executor.cc
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

//psyllid includes
#include "batch_executor.hh"

//non-psyllid P8 includes
#include "dripline_constants.hh"
#include "logger.hh"
#include "message.hh"
#include "request_receiver.hh"

//external includes
#include <chrono>
#include <thread>

namespace psyllid
{

    LOGGER( plog, "batch_executor" );

    batch_executor::batch_executor() :
        f_actions_array(),
        f_request_receiver()
    {
    }

    batch_executor::batch_executor( const scarab::param_node& a_master_config, std::shared_ptr<psyllid::request_receiver> a_request_receiver ) :
        f_actions_array( *a_master_config.array_at( "batch-actions" ) ),
        f_request_receiver( a_request_receiver )
    {
    }

    batch_executor::~batch_executor()
    {
    }


    /* considering yaml that looks like:
    batch-actions:
        - type: cmd
          sleep-for: 500 # [ms], optional element to specify the sleep after issuing the cmd, before proceeding to the next.
          rks: start-run
          payload:
            duration: 200
            filenames: '["/tmp/foo_t.yaml", "/tmp/foo_f.yaml"]'
    */
    void batch_executor::execute()
    {
        for ( scarab::param_array::iterator action_it = f_actions_array.begin();
              action_it!=f_actions_array.end();
              ++action_it )
        {
            LDEBUG( plog, "doing next action:\n" << *action_it );
            // parse/cast useful variables/types from the action node
            scarab::param_node* a_payload = (*action_it)->as_node().node_at( "payload" );
            dripline::op_t a_msg_op = dripline::to_op_t( (*action_it)->as_node().value_at( "type" )->as_string() );
            std::string a_routing_key = std::string(".") + (*action_it)->as_node().value_at( "rks")->as_string();
            unsigned a_sleep = std::stoi( (*action_it)->as_node().get_value( "sleep-for", "500"), nullptr, 10 );
            // put it together into a request
            dripline::request_ptr_t a_request = dripline::msg_request::create(
                                              a_payload,
                                              a_msg_op,
                                              a_routing_key,
                                              std::string("") );// reply-to is empty because no reply for batch reqeusts
            // submit the request object to the receiver and sleep
            f_request_receiver.get()->submit_request_message( a_request );
            std::this_thread::sleep_for( std::chrono::milliseconds( a_sleep ) );
        }
    }

} /* namespace psyllid */
