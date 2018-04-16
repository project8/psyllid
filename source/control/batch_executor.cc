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

using dripline::request_ptr_t;

namespace psyllid
{

    LOGGER( plog, "batch_executor" );

    batch_executor::batch_executor() :
        f_actions_array()
    {
    }

    batch_executor::batch_executor( const scarab::param_node& a_master_config ) :
        f_actions_array( *a_master_config.array_at( "batch-actions" ) )
    {
    }

    batch_executor::~batch_executor()
    {
    }


    /* considering yaml that looks like:
    batch-actions:
        - type: cmd
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
            //access each action (still a node) as *action_it
            // get this from the iterator
            scarab::param* a_payload = (*action_it)->as_node().at( "payload" );
            //TODO add string -> op_t mapping in dripline-cpp
            scarab::param_value a_val = (*action_it)->as_node().value_at( "type" );
            std::string a_str_op_val = (*action_it)->as_node().value_at( "type" )->as_string();
            //dripline::op_t a_msg_op = dripline::to_op_t( (*action_it)->value_at("type").as_string() );
            request_ptr_t a_reqeust = dripline::msg_request::create(
                                              &(a_payload->as_node()),
                                              dripline::op_t::cmd,//a_msg_op, //1, //msg_op [op_t]
                                              std::string(""), //routing key [std::string]
                                              std::string("") );//, //reply-to [std::string]
        }
    }

} /* namespace psyllid */
