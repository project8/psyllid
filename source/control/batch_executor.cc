/*
 * batch_executor.cc
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

//psyllid includes
#include "batch_executor.hh"
#include "daq_control.hh"
#include "request_receiver.hh"

//non-psyllid P8 includes
#include "dripline_constants.hh"
#include "logger.hh"

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
        f_actions_array(),
        f_request_receiver( a_request_receiver )
    {
        if ( a_master_config.has( "batch-actions" ) )
        {
            LWARN( plog, "got array" );
            f_actions_array = *(a_master_config.array_at( "batch-actions" ));
        }
        else
        {
            LINFO( plog, "batch array is null" );
            f_actions_array = scarab::param_array();
        }
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
        // start with a sleep... is request_receiver ready?
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        if ( f_actions_array.is_null() )
        {
            LINFO( plog, "actions array is null" );
            return;
        }

        LDEBUG( plog, "actions array size is: " << f_actions_array.size() );
        for( scarab::param_array::const_iterator action_it = f_actions_array.begin();
              action_it!=f_actions_array.end();
              ++action_it )
        {
            if ( is_canceled() ) break;

            LDEBUG( plog, "doing next action:\n" << **action_it );
            action_info t_action = parse_action( &((*action_it)->as_node()) );

            // submit the request object to the receiver and sleep
            dripline::reply_info t_request_reply_info = f_request_receiver->submit_request_message( t_action.f_request_ptr );
            if ( ! t_request_reply_info )
            {
                LWARN( plog, "failed submitting action request" );
                throw psyllid::error() << "error while submitting command";
            }
            if ( t_action.f_is_custom_action )
            {
                LWARN( plog, "figuring out how to do a polling loop" );
                daq_control::status t_status = daq_control::uint_to_status( t_request_reply_info.f_payload.node_at("server")->value_at("status-value")->as_uint());
                while ( t_status == daq_control::status::running )
                {
                    // update status
                    t_action = parse_action( &((*action_it)->as_node()) );
                    t_request_reply_info = f_request_receiver->submit_request_message( t_action.f_request_ptr );
                    t_status = daq_control::uint_to_status( t_request_reply_info.f_payload.node_at("server")->value_at("status-value")->as_uint());
                    std::this_thread::sleep_for( std::chrono::milliseconds( t_action.f_sleep_duration_ms ) );
                }
            }
            else
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( t_action.f_sleep_duration_ms ) );
            }
        } // loop over actions param_array

        LINFO( plog, "action loop complete" );
    }

    void batch_executor::do_cancellation()
    {
        LDEBUG( plog, "canceling batch executor" );
        return;
    }

    action_info batch_executor::parse_action( const scarab::param_node* a_action )
    {
        action_info t_action_info;
        std::string t_rks;
        dripline::op_t t_msg_op;
        if ( ! a_action->node_at( "payload" )->is_node() )
        {
            LERROR( plog, "payload must be a param_node" );
            throw psyllid::error() << "batch action payload must be a node";
        }
        try
        {
            t_rks = a_action->get_value( "rks");
            t_action_info.f_sleep_duration_ms = std::stoi( a_action->get_value( "sleep-for", "500" ) );
            t_action_info.f_is_custom_action = false;
        }
        catch( scarab::error )
        {
            LERROR( plog, "error parsing action param_node, check keys and value types" );
            throw;
        }
        try
        {
            t_msg_op = dripline::to_op_t( a_action->get_value( "type" ) );
        }
        catch( dripline::dripline_error )
        {
            LDEBUG( plog, "got a dripline error parsing request type" );
            if ( a_action->get_value( "type" ) == "wait-for" && t_rks == "daq-status" )
            {
                LDEBUG( plog, "action is poll on run status" );
                t_msg_op = dripline::op_t::get;
                t_action_info.f_is_custom_action = true;
            }
            else throw;
        }
        LDEBUG( plog, "build request object" );

        // put it together into a request
        t_action_info.f_request_ptr = dripline::msg_request::create(
                                            &(a_action->node_at( "payload" )->clone()->as_node()),
                                            t_msg_op,
                                            std::string(),
                                            std::string() );// reply-to is empty because no reply for batch requests
        t_action_info.f_request_ptr->set_routing_key_specifier( t_rks, dripline::routing_key_specifier( t_rks ) );
        LINFO( plog, "next action will be " << t_action_info.f_request_ptr->get_payload() );
        return t_action_info;
    }

} /* namespace psyllid */
