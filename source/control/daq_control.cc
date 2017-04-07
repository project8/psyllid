/*
 * daq_control.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "daq_control.hh"

#include "butterfly_house.hh"
#include "node_builder.hh"

#include "diptera.hh"
#include "midge_error.hh"

#include "logger.hh"

#include <chrono>
#include <condition_variable>
#include <future>
#include <signal.h>
#include <thread>

using scarab::param_array;
using scarab::param_node;
using scarab::param_value;

using dripline::request_ptr_t;
using dripline::retcode_t;

using std::string;

namespace psyllid
{
    LOGGER( plog, "daq_control" );

    daq_control::daq_control( const param_node& a_master_config, std::shared_ptr< stream_manager > a_mgr ) :
            scarab::cancelable(),
            control_access(),
            f_activation_condition(),
            f_daq_mutex(),
            f_node_manager( a_mgr ),
            f_daq_config( new param_node() ),
            f_midge_pkg(),
            f_node_bindings( nullptr ),
            f_run_stopper(),
            f_run_stop_mutex(),
            f_run_return(),
            f_run_duration( 1000 ),
            f_status( status::deactivated )
    {
        // DAQ config is optional; defaults will work just fine
        const scarab::param_node* t_daq_config = a_master_config.node_at( "daq" );

        if( t_daq_config != nullptr )
        {
            f_daq_config.reset( new param_node( *t_daq_config ) );
        }

        set_run_duration( f_daq_config->get_value( "duration", get_run_duration() ) );
    }

    daq_control::~daq_control()
    {
    }

    void daq_control::initialize()
    {
        butterfly_house::get_instance()->set_daq_control( f_daq_control );
        butterfly_house::get_instance()->prepare_files( f_daq_config.get() );
        return;
    }

    void daq_control::execute()
    {
        // if we're supposed to activate on startup, we'll call activate asynchronously
        std::future< void > t_activation_return;
        if( f_daq_config->get_value< bool >( "activate-at-startup", false ) )
        {
            LDEBUG( plog, "Will activate DAQ control asynchronously" );
            t_activation_return = std::async( std::launch::async,
                        [this]()
                        {
                            std::this_thread::sleep_for( std::chrono::milliseconds(250));
                            LDEBUG( plog, "Activating DAQ control at startup" );
                            activate();
                        } );
        }

        // Errors caught during this loop are handled by setting the status to error, and continuing the loop,
        // which then goes to the error clause of the outer if/elseif logic
        while( ! is_canceled() )
        {
            status t_status = get_status();
            LTRACE( plog, "daq_control execute loop; status is <" << interpret_status( t_status ) << ">" );
            if( t_status == status::deactivated )
            {
                std::unique_lock< std::mutex > t_lock( f_daq_mutex );
                //LDEBUG( plog, "DAQ control deactivated and waiting for activation signal" );
                f_activation_condition.wait_for( t_lock, std::chrono::seconds(1) );
            }
            else if( t_status == status::activating )
            {
                LINFO( plog, "DAQ control activating" );

                try
                {
                    if( f_node_manager->must_reset_midge() )
                    {
                        LDEBUG( plog, "Reseting midge" );
                        f_node_manager->reset_midge();
                    }
                }
                catch( error& e )
                {
                    LWARN( plog, "Exception caught while resetting midge: " << e.what() );
                    LWARN( plog, "Returning to the \"deactivated\" state and awaiting further instructions" );
                    set_status( status::deactivated );
                    continue;
                }

                LDEBUG( plog, "Acquiring midge package" );
                f_midge_pkg = f_node_manager->get_midge();

                if( ! f_midge_pkg.have_lock() )
                {
                    LERROR( plog, "Could not get midge resource" );
                    set_status( status::error );
                    continue;
                }

                f_node_bindings = f_node_manager->get_node_bindings();

                try
                {
                    std::string t_run_string( f_node_manager->get_node_run_str() );
                    LDEBUG( plog, "Starting midge with run string <" << t_run_string << ">" );
                    set_status( status::activated );
                    f_midge_pkg->run( t_run_string );
                    LINFO( plog, "DAQ control is shutting down after midge exited" );
                }
                catch( std::exception& e )
                {
                    LERROR( plog, "An exception was thrown while running midge: " << e.what() );
                    set_status( status::error );
                    f_run_stopper.notify_all();
                    continue;
                }

                f_node_manager->return_midge( std::move( f_midge_pkg ) );
                f_node_bindings = nullptr;

                if( get_status() == status::running )
                {
                    LERROR( plog, "Midge exited abnormally" );
                    set_status( status::error );
                    f_run_stopper.notify_all();
                    continue;
                }
            }
            else if( t_status == status::activated )
            {
                LERROR( plog, "DAQ control status is activated in the outer execution loop!" );
                set_status( status::error );
                continue;
            }
            else if( t_status == status::deactivating )
            {
                LDEBUG( plog, "DAQ control deactivating; status now set to \"deactivated\"" );
                set_status( status::deactivated );
            }
            else if( t_status == status::done )
            {
                LINFO( plog, "Exiting DAQ control" );
                f_node_bindings = nullptr;
                break;
            }
            else if( t_status == status::error )
            {
                LERROR( plog, "DAQ control is in an error state" );
                f_node_bindings = nullptr;
                raise( SIGINT );
                break;
            }
        }
        return;
    }

    void daq_control::activate()
    {
        LDEBUG( plog, "Activating DAQ control" );

        if( is_canceled() )
        {
            throw error() << "DAQ control has been canceled";
        }

        if( get_status() != status::deactivated )
        {
            throw status_error() << "DAQ control is not in the deactivated state";
        }

        set_status( status::activating );
        f_activation_condition.notify_one();

        return;
    }

    void daq_control::reactivate()
    {
        try
        {
            deactivate();
            activate();
        }
        catch( error& e )
        {
            throw e;
        }
        return;
    }

    void daq_control::deactivate()
    {
        LDEBUG( plog, "Deactivating DAQ" );

        if( is_canceled() )
        {
            throw error() << "DAQ control has been canceled";
        }

        if( get_status() != status::activated )
        {
            throw status_error() << "Invalid state for deactivating: <" + daq_control::interpret_status( get_status() ) + ">; DAQ control must be in activated state";
        }

        set_status( status::deactivating );
        if( f_midge_pkg.have_lock() )
        {
            LDEBUG( plog, "Canceling DAQ worker from DAQ control" );
            f_midge_pkg->cancel();
        }

        return;
    }

    void daq_control::start_run()
    {
        LDEBUG( plog, "Preparing for run" );

        if( is_canceled() )
        {
            throw error() << "daq_control has been canceled";
        }

        if( get_status() != status::activated )
        {
            throw status_error() << "DAQ control must be in the activated state to start a run; activate the DAQ and try again";
        }

        if( ! f_midge_pkg.have_lock() )
        {
            throw error() << "Do not have midge resource";
        }

        LDEBUG( plog, "Launching asynchronous do_run" );
        f_run_return = std::async( std::launch::async, &daq_control::do_run, this, f_run_duration );
        //TODO: use run return?

        return;
    }

    void daq_control::do_run( unsigned a_duration )
    {
        LINFO( plog, "Run is commencing" );

        LDEBUG( plog, "Starting egg files" );
        try
        {
            butterfly_house::get_instance()->start_files();
        }
        catch( error& e )
        {
            LERROR( plog, "Unable to start files: " << e.what() );
            set_status( status::error );
            LDEBUG( plog, "Canceling midge" );
            if( f_midge_pkg.have_lock() ) f_midge_pkg->cancel();
            return;
        }

        const unsigned t_sub_duration = 100;
        unsigned t_n_sub_durations = 0;
        unsigned t_duration_remainder = 0;
        if( a_duration != 0 )
        {
            t_n_sub_durations = a_duration / t_sub_duration; // number of complete sub-durations, not including the remainder
            t_duration_remainder = a_duration - t_n_sub_durations * t_sub_duration;
        }

        std::unique_lock< std::mutex > t_run_stop_lock( f_run_stop_mutex );

        set_status( status::running );
        f_midge_pkg->instruct( midge::instruction::resume );

        std::cv_status t_wait_return = std::cv_status::timeout;

        if( a_duration == 0 )
        {
            LDEBUG( plog, "Untimed run stopper in use" );
            f_run_stopper.wait( t_run_stop_lock );
            // conditions that will break the loop:
            //   - last sub-duration was not stopped for a timeout (the other possibility is that f_run_stopper was notified by e.g. stop_run())
            //   - daq_control has been canceled
            while( t_wait_return == std::cv_status::timeout && ! is_canceled() )
            {
                t_wait_return = f_run_stopper.wait_for( t_run_stop_lock, std::chrono::milliseconds( t_sub_duration ) );
            }
        }
        else
        {
            LDEBUG( plog, "Timed run stopper in use; limit is " << a_duration << " ms" );
            // all but the last sub-duration last for t_sub_duration ms
            // conditions that will break the loop:
            //   - all sub-durations have been completed
            //   - last sub-duration was not stopped for a timeout (the other possibility is that f_run_stopper was notified by e.g. stop_run())
            //   - daq_control has been canceled
            for( unsigned i_sub_duration = 0;
                    i_sub_duration < t_n_sub_durations && t_wait_return == std::cv_status::timeout && ! is_canceled();
                    ++i_sub_duration )
            {
                t_wait_return = f_run_stopper.wait_for( t_run_stop_lock, std::chrono::milliseconds( t_sub_duration ) );
            }
            // the last sub-duration is for a different amount of time (t_duration_remainder) than the standard sub-duration (t_sub_duration)
            if( t_wait_return == std::cv_status::timeout && ! is_canceled() )
            {
                t_wait_return = f_run_stopper.wait_for( t_run_stop_lock, std::chrono::milliseconds( t_duration_remainder ) );
            }
        }

        // if we've reached here, we need to pause midge.
        // reasons for this include the timer has run out in a timed run, or the run has been manually stopped

        LDEBUG( plog, "Run stopper has been released" );

        f_midge_pkg->instruct( midge::instruction::pause );
        set_status( status::activated );

        LINFO( plog, "Run has stopped" );

        // give midge time to finish the streams before finishing the files
        std::this_thread::sleep_for( std::chrono::milliseconds(500));
        LDEBUG( plog, "Finishing egg files" );
        butterfly_house::get_instance()->finish_files();

        // if daq_control has been canceled, assume that midge has been canceled by other methods, and this function should just exit
        if( is_canceled() )
        {
            LDEBUG( plog, "Run was cancelled" );
        }

        return;
    }

    void daq_control::stop_run()
    {
        LINFO( plog, "Run stop requested" );

        if( get_status() != status::running ) return;

        if( ! f_midge_pkg.have_lock() )
        {
            throw error() << "Do not have midge resource; worker is not currently running";
        }

        std::unique_lock< std::mutex > t_run_stop_lock( f_run_stop_mutex );
        f_run_stopper.notify_all();

        return;
    }

    void daq_control::do_cancellation()
    {
        LDEBUG( plog, "Canceling DAQ control" );
        set_status( status::canceled );

        if( get_status() == status::running )
        {
            LDEBUG( plog, "Canceling run" );
            try
            {
                stop_run();
            }
            catch( error& e )
            {
                LERROR( plog, "Unable to complete stop-run: " << e.what() );
            }
        }

        LDEBUG( plog, "Canceling midge" );
        if( f_midge_pkg.have_lock() ) f_midge_pkg->cancel();

        return;
    }

    void daq_control::apply_config( const std::string& a_node_name, const scarab::param_node& a_config )
    {
        if( f_node_bindings == nullptr )
        {
            throw error() << "Can't apply config to node <" << a_node_name << ">: node bindings aren't available";
        }

        active_node_bindings::iterator t_binding_it = f_node_bindings->find( a_node_name );
        if( t_binding_it == f_node_bindings->end() )
        {
            throw error() << "Can't apply config to node <" << a_node_name << ">: did not find node";
        }

        try
        {
            LDEBUG( plog, "Applying config to active node <" << a_node_name << ">: " << a_config );
            t_binding_it->second.first->apply_config( t_binding_it->second.second, a_config );
        }
        catch( std::exception& e )
        {
            throw error() << "Can't apply config to node <" << a_node_name << ">: " << e.what();
        }
        return;
    }

    void daq_control::dump_config( const std::string& a_node_name, scarab::param_node& a_config )
    {
        if( f_node_bindings == nullptr )
        {
            throw error() << "Can't dump config from node <" << a_node_name << ">: node bindings aren't available";
        }

        if( f_node_bindings == nullptr )
        {
            throw error() << "Can't dump config from node <" << a_node_name << ">: node bindings aren't available";
        }

        active_node_bindings::iterator t_binding_it = f_node_bindings->find( a_node_name );
        if( t_binding_it == f_node_bindings->end() )
        {
            throw error() << "Can't dump config from node <" << a_node_name << ">: did not find node";
        }

        try
        {
            LDEBUG( plog, "Dumping config from active node <" << a_node_name << ">" );
            t_binding_it->second.first->dump_config( t_binding_it->second.second, a_config );
        }
        catch( std::exception& e )
        {
            throw error() << "Can't dump config from node <" << a_node_name << ">: " << e.what();
        }
        return;
    }

    bool daq_control::run_command( const std::string& a_node_name, const std::string& a_cmd, const scarab::param_node& a_args )
    {
        if( f_node_bindings == nullptr )
        {
            throw error() << "Can't run command <" << a_cmd << "> on node <" << a_node_name << ">: node bindings aren't available";
        }

        if( f_node_bindings == nullptr )
        {
            throw error() << "Can't run command <" << a_cmd << "> on node <" << a_node_name << ">: node bindings aren't available";
        }

        active_node_bindings::iterator t_binding_it = f_node_bindings->find( a_node_name );
        if( t_binding_it == f_node_bindings->end() )
        {
            throw error() << "Can't run command <" << a_cmd << "> on node <" << a_node_name << ">: did not find node";
        }

        try
        {
            LDEBUG( plog, "Running command <" << a_cmd << "> on active node <" << a_node_name << ">" );
            return t_binding_it->second.first->run_command( t_binding_it->second.second, a_cmd, a_args );
        }
        catch( std::exception& e )
        {
            throw error() << "Can't run command <" << a_cmd << "> on node <" << a_node_name << ">: " << e.what();
        }
    }


    bool daq_control::handle_activate_daq_control( const request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            activate();
            return a_reply_pkg.send_reply( retcode_t::success, "DAQ control activated" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to activate DAQ control: " ) + e.what() );
        }
    }

    bool daq_control::handle_reactivate_daq_control( const dripline::request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            reactivate();
            return a_reply_pkg.send_reply( retcode_t::success, "DAQ control reactivated" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to reactivate DAQ control: " ) + e.what() );
        }
    }

    bool daq_control::handle_deactivate_daq_control( const request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            deactivate();
            return a_reply_pkg.send_reply( retcode_t::success, "DAQ control deactivated" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to deactivate DAQ control: " ) + e.what() );
        }
    }

    bool daq_control::handle_start_run_request( const request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            f_run_filename = a_request->get_payload().get_value( "filename", f_run_filename );
            f_run_description = a_request->get_payload().get_value( "description", f_run_description );
            f_run_duration = a_request->get_payload().get_value( "duration", f_run_duration );
            start_run();
            return a_reply_pkg.send_reply( retcode_t::success, "Run started" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to start run: " ) + e.what() );
        }
    }

    bool daq_control::handle_stop_run_request( const request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            stop_run();
            return a_reply_pkg.send_reply( retcode_t::success, "Run stopped" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to stop run: " ) + e.what() );
        }
    }

    bool daq_control::handle_apply_config_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( a_request->parsed_rks().size() < 2 )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "RKS is improperly formatted: [queue].active-config.[stream].[node] or [queue].node-config.[stream].[node].[parameter]" );
        }

        //size_t t_rks_size = a_request->parsed_rks().size();

        std::string t_target_stream = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        std::string t_target_node = t_target_stream + "_" + a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        if( a_request->parsed_rks().empty() )
        {
            // payload should be a map of all parameters to be set
            LDEBUG( plog, "Performing config for multiple values in active node <" << t_target_node << ">" );

            if( a_request->get_payload().empty() )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Unable to perform active-config request: payload is empty" );
            }

            try
            {
                apply_config( t_target_node, a_request->get_payload() );
                a_reply_pkg.f_payload.merge( a_request->get_payload() );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform node-config request: ") + e.what() );
            }
        }
        else
        {
            // payload should be values array with a single entry for the particular parameter to be set
            LDEBUG( plog, "Performing node config for a single value in active node <" << t_target_node << ">" );

            if( ! a_request->get_payload().has( "values" ) )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Unable to perform active-config (single value): values array is missing" );
            }
            const scarab::param_array* t_values_array = a_request->get_payload().array_at( "values" );
            if( t_values_array == nullptr || t_values_array->empty() || ! (*t_values_array)[0].is_value() )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Unable to perform active-config (single value): \"values\" is not an array, or the array is empty, or the first element in the array is not a value" );
            }

            scarab::param_node t_param_to_set;
            t_param_to_set.add( a_request->parsed_rks().front(), new scarab::param_value( (*t_values_array)[0].as_value() ) );

            try
            {
                apply_config( t_target_node, t_param_to_set );
                a_reply_pkg.f_payload.merge( t_param_to_set );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform active-config request (single value): ") + e.what() );
            }
        }

        LDEBUG( plog, "Node-config was successful" );
        return a_reply_pkg.send_reply( dripline::retcode_t::success, "Performed node-config" );
    }

    bool daq_control::handle_dump_config_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( a_request->parsed_rks().size() < 2 )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "RKS is improperly formatted: [queue].active-config.[stream].[node] or [queue].active-config.[stream].[node].[parameter]" );
        }

        //size_t t_rks_size = a_request->parsed_rks().size();

        std::string t_target_stream = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        std::string t_target_node = t_target_stream + "_" + a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        if( a_request->parsed_rks().empty() )
        {
            // getting full node configuration
            LDEBUG( plog, "Getting node config for active node <" << t_target_node << ">" );

            try
            {
                dump_config( t_target_node, a_reply_pkg.f_payload );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform get-active-config request: ") + e.what() );
            }
        }
        else
        {
            // getting value for a single parameter
            LDEBUG( plog, "Getting value for a single parameter in active node <" << t_target_node << ">" );

            std::string t_param_to_get = a_request->parsed_rks().front();

            try
            {
                scarab::param_node t_param_dump;
                dump_config( t_target_node, t_param_dump );
                if( ! t_param_dump.has( t_param_to_get ) )
                {
                    return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "Unable to get active-node parameter: cannot find parameter <" + t_param_to_get + ">" );
                }
                a_reply_pkg.f_payload.add( t_param_to_get, new scarab::param_value( *t_param_dump.value_at( t_param_to_get ) ) );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to get active-node parameter (single value): ") + e.what() );
            }
        }

        LDEBUG( plog, "Get-active-node-config was successful" );
        return a_reply_pkg.send_reply( dripline::retcode_t::success, "Performed get-active-node-config" );
    }

    bool daq_control::handle_run_command_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( a_request->parsed_rks().size() < 2 )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "RKS is improperly formatted: [queue].run-command.[stream].[node].[command]" );
        }

        //size_t t_rks_size = a_request->parsed_rks().size();

        std::string t_target_stream = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        std::string t_target_node = t_target_stream + "_" + a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        scarab::param_node t_args_node( a_request->get_payload() );
        std::string t_command( a_request->parsed_rks().front() );
        a_request->parsed_rks().pop_front();

        LDEBUG( plog, "Performing run-command <" << t_command << "> for active node <" << t_target_node << ">; args:\n" << t_args_node );

        bool t_return = false;
        try
        {
            t_return = run_command( t_target_node, t_command, t_args_node );
            a_reply_pkg.f_payload.merge( t_args_node );
            a_reply_pkg.f_payload.add( "command", new scarab::param_value( t_command ) );
        }
        catch( std::exception& e )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform run-command request: ") + e.what() );
        }

        if( t_return )
        {
            LDEBUG( plog, "Active run-command execution was successful" );
            return a_reply_pkg.send_reply( dripline::retcode_t::success, "Performed active run-command execution" );
        }
        else
        {
            LWARN( plog, "Active run-command execution failed" );
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_method, "Command was not recognized" );
        }
    }

    bool daq_control::handle_set_filename_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_rks().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_rks().front() );
            }

            std::string t_filename =  a_request->get_payload().array_at( "values" )->get_value( 0 );
            LDEBUG( plog, "Setting filename for file <" << t_file_num << "> to <" << t_filename << ">" );
            set_filename( t_filename, t_file_num );
            return a_reply_pkg.send_reply( retcode_t::success, "Filename set" );
        }
        catch( std::exception& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to set filename: " ) + e.what() );
        }
    }

    bool daq_control::handle_set_description_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_rks().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_rks().front() );
            }

            std::string t_description =  a_request->get_payload().array_at( "values" )->get_value( 0 );
            LDEBUG( plog, "Setting description for file <" << t_file_num << "> to <" << t_description << ">" );
            set_filename( t_description, t_file_num );
            return a_reply_pkg.send_reply( retcode_t::success, "Description set" );
        }
        catch( scarab::error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to set description: " ) + e.what() );
        }
    }

    bool daq_control::handle_set_duration_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            f_run_duration =  a_request->get_payload().array_at( "values" )->get_value< unsigned >( 0 );
            LDEBUG( plog, "Duration set to <" << f_run_duration << "> ms" );
            return a_reply_pkg.send_reply( retcode_t::success, "Duration set" );
        }
        catch( scarab::error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to set duration: " ) + e.what() );
        }
    }

    bool daq_control::handle_get_status_request( const dripline::request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        param_node* t_server_node = new param_node();
        t_server_node->add( "status", new param_value( interpret_status( get_status() ) ) );
        t_server_node->add( "status-value", new param_value( status_to_uint( get_status() ) ) );

        // TODO: add status of nodes

        a_reply_pkg.f_payload.add( "server", t_server_node );

        return a_reply_pkg.send_reply( retcode_t::success, "DAQ status request succeeded" );

    }

    bool daq_control::handle_get_filename_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_rks().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_rks().front() );
            }

            param_array* t_values_array = new param_array();
            t_values_array->push_back( new param_value( get_filename( t_file_num ) ) );
            a_reply_pkg.f_payload.add( "values", t_values_array );
            return a_reply_pkg.send_reply( retcode_t::success, "Description set" );
        }
        catch( scarab::error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to set description: " ) + e.what() );
        }

        return a_reply_pkg.send_reply( retcode_t::success, "Filename request completed" );
    }

    bool daq_control::handle_get_description_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_rks().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_rks().front() );
            }

            param_array* t_values_array = new param_array();
            t_values_array->push_back( new param_value( get_description( t_file_num ) ) );
            a_reply_pkg.f_payload.add( "values", t_values_array );
            return a_reply_pkg.send_reply( retcode_t::success, "Description set" );
        }
        catch( scarab::error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to set description: " ) + e.what() );
        }

        return a_reply_pkg.send_reply( retcode_t::success, "Description request completed" );
    }

    bool daq_control::handle_get_duration_request( const dripline::request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        param_array* t_values_array = new param_array();
        t_values_array->push_back( new param_value( f_run_duration ) );

        a_reply_pkg.f_payload.add( "values", t_values_array );

        return a_reply_pkg.send_reply( retcode_t::success, "Duration request completed" );
    }


    void daq_control::set_filename( const std::string& a_filename, unsigned a_file_num )
    {
        try
        {
            butterfly_house::get_instance()->set_filename( a_filename, a_file_num );
            return;
        }
        catch( error& )
        {
            throw;
        }
    }

    const std::string& daq_control::get_filename( unsigned a_file_num )
    {
        try
        {
            return butterfly_house::get_instance()->get_filename( a_file_num );
        }
        catch( error& )
        {
            throw;
        }
    }

    void daq_control::set_description( const std::string& a_desc, unsigned a_file_num )
    {
        try
        {
            butterfly_house::get_instance()->set_description( a_desc, a_file_num );
            return;
        }
        catch( error& )
        {
            throw;
        }
    }

    const std::string& daq_control::get_description( unsigned a_file_num )
    {
        try
        {
            return butterfly_house::get_instance()->get_description( a_file_num );
        }
        catch( error& )
        {
            throw;
        }
    }

    uint32_t daq_control::status_to_uint( status a_status )
    {
        return static_cast< uint32_t >( a_status );
    }
    daq_control::status daq_control::uint_to_status( uint32_t a_value )
    {
        return static_cast< status >( a_value );
    }
    std::string daq_control::interpret_status( status a_status )
    {
        switch( a_status )
        {
            case status::deactivated:
                return std::string( "Deactivated" );
                break;
            case status::activating:
                return std::string( "Activating" );
                break;
            case status::activated:
                return std::string( "Activated" );
                break;
            case status::running:
                return std::string( "Running" );
                break;
            case status::deactivating:
                return std::string( "Deactivating" );
                break;
            case status::canceled:
                return std::string( "Canceled" );
                break;
            case status::done:
                return std::string( "Done" );
                break;
            case status::error:
                return std::string( "Error" );
                break;
            default:
                return std::string( "Unknown" );
        }
    }


} /* namespace psyllid */
