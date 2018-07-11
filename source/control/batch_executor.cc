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
#include "dripline_error.hh"
#include "logger.hh"

//external includes
#include <chrono>
#include <thread>

namespace psyllid
{

    LOGGER( plog, "batch_executor" );

    batch_executor::batch_executor() :
        f_batch_commands(),
        f_request_receiver(),
        f_action_queue(),
        f_condition_actions()
    {
    }

    batch_executor::batch_executor( const scarab::param_node& a_master_config, std::shared_ptr<psyllid::request_receiver> a_request_receiver ) :
        f_batch_commands( *(a_master_config.node_at( "batch-commands" )) ),
        f_request_receiver( a_request_receiver ),
        f_action_queue(),
        f_condition_actions()
    {
        if ( a_master_config.has( "batch-actions" ) )
        {
            LINFO( plog, "have an initial action array" );
            add_to_queue( a_master_config.array_at( "batch-actions" ) );
        }
        else
        {
            LINFO( plog, "no initial batch actions" );
        }
        if ( a_master_config.has( "settable-conditions" ) )
        {
            LINFO( plog, "storing setable conditions mapping" );
            if ( !a_master_config.node_at( "settable-conditions" ) ) {
                // settable-conditions is not a node, throw an exception
            }
        }

        // register batch commands
        using namespace std::placeholders;
        for ( scarab::param_node::iterator command_it = f_batch_commands.begin(); command_it != f_batch_commands.end(); command_it++ )
        {
            a_request_receiver->register_cmd_handler( command_it->first, std::bind( &batch_executor::do_batch_cmd_request, this, command_it->first, _1, _2 ) );
        }
    }

    batch_executor::~batch_executor()
    {
    }

    void batch_executor::clear_queue()
    {
        action_info t_action;
        while ( f_action_queue.try_pop( t_action ) )
        {
        }
    }

    void batch_executor::add_to_queue( const scarab::param_node* an_action )
    {
        f_action_queue.push( parse_action( an_action ) );
    }

    void batch_executor::add_to_queue( const scarab::param_array* actions_array )
    {
        for( scarab::param_array::const_iterator action_it = actions_array->begin();
              action_it!=actions_array->end();
              ++action_it )
        {
            //TODO reduce or remove
            LWARN( plog, "adding an item: " << (*action_it)->as_node())
            add_to_queue( &((*action_it)->as_node()) );
        }
    }

    void batch_executor::add_to_queue( const std::string a_batch_command_name )
    {
        if ( f_batch_commands.has( a_batch_command_name ) )
        {
            add_to_queue( f_batch_commands.array_at( a_batch_command_name ) );
        }
        else
        {
            LWARN( plog, "no configured batch action for: '" << a_batch_command_name << "' ignoring" );
        }
    }

    void batch_executor::replace_queue( const scarab::param_node* an_action )
    {
        clear_queue();
        add_to_queue( an_action );
    }

    void batch_executor::replace_queue( const scarab::param_array* actions_array )
    {
        clear_queue();
        add_to_queue( actions_array );
    }

    void batch_executor::replace_queue( const std::string a_batch_command_name )
    {
        clear_queue();
        add_to_queue( a_batch_command_name );
    }

    // this method should be bound in the request receiver to be called with a command name, the request_ptr_t is not used
    dripline::reply_info batch_executor::do_batch_cmd_request( const std::string& a_command, const dripline::request_ptr_t /* a_request_ptr_t */, dripline::reply_package& a_reply_package )
    {
        try
        {
            add_to_queue( a_command );
        }
        catch( dripline::dripline_error& e )
        {
            return a_reply_package.send_reply( dripline::retcode_t::daq_error, std::string("Error processing command: ") + e.what() );
        }
        return a_reply_package.send_reply( dripline::retcode_t::success, "" );
    }

    // this method should be bound in the request receiver to be called with a command name, the request_ptr_t is not used
    dripline::reply_info batch_executor::do_replace_actions_request( const std::string& a_command, const dripline::request_ptr_t /* a_request_ptr_t */, dripline::reply_package& a_reply_package )
    {
        try
        {
            //TODO do we have a good way to interrupt an ongoing action here?
            replace_queue( a_command );
        }
        catch( dripline::dripline_error& e )
        {
            return a_reply_package.send_reply( dripline::retcode_t::daq_error, std::string("Error processing command: ") + e.what() );
        }
        return a_reply_package.send_reply( dripline::retcode_t::success, "" );
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
    void batch_executor::execute( bool run_forever )
    {
        while ( run_forever || f_action_queue.size() )
        {
            if ( is_canceled() ) break;
            if ( f_action_queue.size() )
            {
                //TODO remove or lower level
                LWARN( plog, "there are: >= " << f_action_queue.size() << " actions remaining" );
                do_an_action();
            }
        }

        LINFO( plog, "action loop complete" );
    }

    void batch_executor::do_an_action()
    {
        action_info t_action;
        if ( !f_action_queue.try_pop( t_action ) )
        {
            LDEBUG( plog, "there are no actions in the queue" );
            return;
        }
        dripline::reply_info t_request_reply_info = f_request_receiver->submit_request_message( t_action.f_request_ptr );
        if ( ! t_request_reply_info )
        {
            LWARN( plog, "failed submitting action request" );
            throw psyllid::error() << "error while submitting command";
        }
        // wait until daq status is no longer "running"
        if ( t_action.f_is_custom_action )
        {
            daq_control::status t_status = daq_control::uint_to_status( t_request_reply_info.f_payload.node_at("server")->value_at("status-value")->as_uint());
            while ( t_status == daq_control::status::running )
            {
                t_request_reply_info = f_request_receiver->submit_request_message( t_action.f_request_ptr );
                t_status = daq_control::uint_to_status( t_request_reply_info.f_payload.node_at("server")->value_at("status-value")->as_uint());
                std::this_thread::sleep_for( std::chrono::milliseconds( t_action.f_sleep_duration_ms ) );
            }
        }
        else
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( t_action.f_sleep_duration_ms ) );
        }
        if ( dripline::to_uint(t_request_reply_info.f_return_code) >= 100 )
        {
            LWARN( plog, "batch action received an error-level return code; exiting" );
            throw psyllid::error() << "error completing batch action, received code [" <<
                                   t_request_reply_info.f_return_code << "]: \"" <<
                                   t_request_reply_info.f_return_msg << "\"";
        }
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
