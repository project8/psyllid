/*
 * daq_control.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "daq_control.hh"

#include "node_manager.hh"

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

    daq_control::daq_control( const param_node& a_master_config, std::shared_ptr< node_manager > a_mgr ) :
            scarab::cancelable(),
            f_activation_condition(),
            f_daq_mutex(),
            f_node_manager( a_mgr ),
            f_daq_config( new param_node() ),
            f_midge_pkg(),
            f_run_stopper(),
            f_run_stop_mutex(),
            f_run_return(),
            f_run_filename( "default_filename_dc.egg" ),
            f_run_description( "default_description" ),
            f_run_duration( 1000 ),
            f_status( status::initialized )
    {
        // DAQ config is optional; defaults will work just fine
        if( a_master_config.has( "daq" ) )
        {
            f_daq_config.reset( new param_node( *a_master_config.node_at( "daq" ) ) );
        }
    }

    daq_control::~daq_control()
    {
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
            if( t_status == status::initialized )
            {
                std::unique_lock< std::mutex > t_lock( f_daq_mutex );
                //LDEBUG( plog, "DAQ control initialized and waiting for activation signal" );
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
                    LERROR( plog, "Exception caught while resetting midge: " << e.what() );
                    set_status( status::error );
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

                try
                {
                    std::string t_run_string( f_node_manager->get_node_run_str() );
                    LDEBUG( plog, "Starting midge with run string <" << t_run_string << ">" );
                    set_status( status::idle );
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

                if( get_status() == status::running )
                {
                    LERROR( plog, "Midge exited abnormally" );
                    set_status( status::error );
                    f_run_stopper.notify_all();
                    continue;
                }
            }
            else if( t_status == status::idle )
            {
                LERROR( plog, "DAQ control status is idle in the outer execution loop!" );
                set_status( status::error );
                continue;
            }
            else if( t_status == status::deactivating )
            {
                LDEBUG( plog, "DAQ control deactivating; status now set to \"initialized\"" );
                set_status( status::initialized );
            }
            else if( t_status == status::done )
            {
                LINFO( plog, "Exiting DAQ control" );
                break;
            }
            else if( t_status == status::error )
            {
                LERROR( plog, "DAQ control is in an error state" );
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

        if( get_status() != status::initialized )
        {
            throw status_error() << "DAQ control is not in the initialized state";
        }

        set_status( status::activating );
        f_activation_condition.notify_one();

        return;
    }

    void daq_control::deactivate()
    {
        LDEBUG( plog, "Deactivating DAQ" );

        if( is_canceled() )
        {
            throw error() << "DAQ control has been canceled";
        }

        if( get_status() != status::idle )
        {
            throw status_error() << "Invalid state for deactivating: <" + daq_control::interpret_status( get_status() ) + ">; DAQ control must be in idle state";
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

        if( get_status() != status::idle )
        {
            throw status_error() << "DAQ control must be in the idle state to start a run; activate the DAQ and try again";
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

        std::unique_lock< std::mutex > t_run_stop_lock( f_run_stop_mutex );

        set_status( status::running );
        f_midge_pkg->instruct( midge::instruction::resume );

        if( a_duration == 0 )
        {
            LDEBUG( plog, "Untimed run stopper in use" );
            f_run_stopper.wait( t_run_stop_lock );
        }
        else
        {
            LDEBUG( plog, "Timed run stopper in use; limit is " << a_duration << " ms" );
            f_run_stopper.wait_for( t_run_stop_lock, std::chrono::milliseconds( a_duration ) );
        }
        LDEBUG( plog, "Run stopper has been released" );

        f_midge_pkg->instruct( midge::instruction::pause );
        set_status( status::idle );

        LINFO( plog, "Run has stopped" );

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

        LDEBUG( plog, "Canceling midge" );
        if( f_midge_pkg.have_lock() ) f_midge_pkg->cancel();
        return;



        return;
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

    bool daq_control::handle_start_run_request( const request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        try
        {
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

    bool daq_control::handle_set_filename_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            f_run_filename =  a_request->get_payload().array_at( "values" )->get_value( 0 );
            LDEBUG( plog, "Run filename set to <" << f_run_filename << ">" );
            return a_reply_pkg.send_reply( retcode_t::success, "Filename set" );
        }
        catch( scarab::error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to set filename: " ) + e.what() );
        }
    }

    bool daq_control::handle_set_description_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        try
        {
            f_run_description =  a_request->get_payload().array_at( "values" )->get_value( 0 );
            LDEBUG( plog, "Run description set to:\n" << f_run_description );
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

    bool daq_control::handle_get_filename_request( const dripline::request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        param_array* t_values_array = new param_array();
        t_values_array->push_back( new param_value( f_run_filename ) );

        a_reply_pkg.f_payload.add( "values", t_values_array );

        return a_reply_pkg.send_reply( retcode_t::success, "Filename request completed" );
    }

    bool daq_control::handle_get_description_request( const dripline::request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        param_array* t_values_array = new param_array();
        t_values_array->push_back( new param_value( f_run_description ) );

        a_reply_pkg.f_payload.add( "values", t_values_array );

        return a_reply_pkg.send_reply( retcode_t::success, "Description request completed" );
    }

    bool daq_control::handle_get_duration_request( const dripline::request_ptr_t, dripline::reply_package& a_reply_pkg )
    {
        param_array* t_values_array = new param_array();
        t_values_array->push_back( new param_value( f_run_duration ) );

        a_reply_pkg.f_payload.add( "values", t_values_array );

        return a_reply_pkg.send_reply( retcode_t::success, "Duration request completed" );
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
            case status::initialized:
                return std::string( "Initialized" );
                break;
            case status::activating:
                return std::string( "Activating" );
                break;
            case status::idle:
                return std::string( "Idle" );
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
