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
        f_actions_node()
    {
    }

    batch_executor::batch_executor( const scarab::param_node& a_master_config ) :
        f_actions_node( *a_master_config.node_at( "batch-actions" ) )
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
            - duration: 200
            - filenames: '["/tmp/foo_t.yaml", "/tmp/foo_f.yaml"]'
    */
    void batch_executor::execute()
    {
        if ( ! f_actions_node.is_array() )
        {
            LWARN( plog, "batch-actions configuration is not an array" );
            return
        }
        for ( scarab::param_array::iterator action_it = f_actions_node.as_array().begin();
              action_it!=f_actions_node.as_array().end();
              ++action_it )
        {
            //access each action (still a node) as *action_it
            // get this from the iterator
            scarab::param_node* a_payload = (*action_it)->as_node().at( "payload" );
            // is there a map for strings to dripline::op_t? should dripline have one?
            dripline::op_t a_msg_op = dripline::op_t::cmd;
            request_ptr_t a_reqeust = dripline::msg_request::create(
                                              a_payload, //*action_it->as_node()->at( "payload" ),//payload
                                              a_msg_op, //1, //msg_op [op_t]
                                              "", //routing key [std::string]
                                              "" );//, //reply-to [std::string]
        }
    }

} /* namespace psyllid */
