/*
 * batch_executor.cc
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

//psyllid includes
#include "batch_executor.hh"
#include "daq_control.hh"

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
            LWARN( plog, "batch array is null" );
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

            // parse/cast useful variables/types from the action node
            const scarab::param_node& t_action = (*action_it)->as_node();
            scarab::param_node* t_payload;
            std::string t_rks;
            unsigned t_sleep;
            dripline::op_t t_msg_op;
            bool t_do_custom_cmd;
            try
            {
                t_payload = &t_action.node_at( "payload" )->clone()->as_node();
                t_rks = t_action.get_value( "rks");
                t_sleep = std::stoi( t_action.get_value( "sleep-for", "500" ) );
                t_do_custom_cmd = false;
            }
            catch( scarab::error )
            {
                LERROR( plog, "error parsing action param_node, check keys and value types" );
                throw;
            }
            try
            {
                t_msg_op = dripline::to_op_t( t_action.get_value( "type" ) );
            }
            catch( dripline::dripline_error )
            {
                LDEBUG( plog, "got a dripline error parsing request type" );
                if ( t_action.get_value( "type" ) == "wait-for" && t_rks == "daq-status" )
                {
                    LDEBUG( plog, "action is poll on run status" );
                    t_msg_op = dripline::op_t::get;
                    t_do_custom_cmd = true;
                }
                else throw;
            }
            LDEBUG( plog, "build request object" );

            // put it together into a request
            dripline::request_ptr_t t_request = dripline::msg_request::create(
                                              t_payload,
                                              t_msg_op,
                                              std::string(),
                                              std::string() );// reply-to is empty because no reply for batch requests
            // submit the request object to the receiver and sleep
            LDEBUG( plog, "from batch exe, request rk is: " << t_request->routing_key() );
            //a_request->rks() = "start-run";
            t_request->set_routing_key_specifier( t_rks, dripline::routing_key_specifier( t_rks ) );
            dripline::reply_info t_request_reply_info = f_request_receiver->submit_request_message( t_request );
            if ( ! t_request_reply_info )
            {
                LWARN( plog, "failed submitting action request" );
                throw psyllid::error() << "error while submitting command";
            }
            if ( t_do_custom_cmd )
            {
                LWARN( plog, "figuring out how to do a polling loop" );
                daq_control::status t_status = daq_control::uint_to_status( t_request_reply_info.f_payload.node_at("server")->value_at("status-value")->as_uint());
                //TODO don't do these
                LERROR( plog, "status is: " << daq_control::status_to_uint(t_status) );
                LERROR( plog, "or more readably: " << daq_control::interpret_status( t_status ) );
                std::this_thread::sleep_for( std::chrono::milliseconds( 6000 ) );
                //while ( f_request_receiver->submit_request_message( t_request ) // no reply payload...
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( t_sleep ) );
        } // loop over actions param_array

        LINFO( plog, "action loop complete" );
    }

    void batch_executor::do_cancellation()
    {
        LDEBUG( plog, "canceling batch executor" );
        return;
    }

} /* namespace psyllid */
