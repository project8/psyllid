/*
 * daq_control.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "daq_control.hh"

#include "daq_worker.hh"
#include "node_manager.hh"

#include "logger.hh"

#include <chrono>
#include <condition_variable>
#include <future>
#include <thread>

using scarab::param_node;

using dripline::request_ptr_t;
using dripline::hub;
using dripline::retcode_t;

using std::string;

namespace psyllid
{
    LOGGER( plog, "daq_control" );

    daq_control::daq_control( const param_node& a_master_config, std::shared_ptr< node_manager > a_mgr ) :
            cancelable(),
            f_condition(),
            f_daq_mutex(),
            f_node_manager( a_mgr ),
            f_daq_worker(),
            f_worker_mutex(),
            f_worker_thread(),
            f_daq_config( new param_node() ),
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

    void daq_control::execute( std::exception_ptr a_ex_ptr )
    {
        // if we're supposed to activate on startup, we'll call activate asynchronously
        if( f_daq_config->get_value< bool >( "activate-on-startup", false ) )
        {
            std::async( std::launch::async,
                        [this]()
                        {
                            std::this_thread::sleep_for( std::chrono::milliseconds(250));
                            DEBUG( plog, "Activating DAQ control at startup" );
                            activate();
                        } );
        }

        while( ! f_canceled.load() )
        {
            status t_status = get_status();
            if( t_status == status::initialized )
            {
                std::unique_lock< std::mutex > t_lock( f_daq_mutex );
                f_condition.wait_for( t_lock, std::chrono::seconds(1) );
            }
            else if( t_status == status::activating )
            {
                std::unique_lock< std::mutex > t_lock( f_worker_mutex );

                if( f_daq_worker )
                {
                    throw error() << "DAQ worker already exists for some unknown reason";
                }

                DEBUG( plog, "Creating new DAQ worker" );
                f_daq_worker.reset( new daq_worker( ) );

                std::exception_ptr t_worker_ex_ptr;
                std::function< void() > t_notifier = [this](){ notify_run_stopped(); };

                DEBUG( plog, "Starting DAQ worker" );
                f_worker_thread = std::thread( &daq_worker::execute, f_daq_worker.get(), f_node_manager, t_worker_ex_ptr, t_notifier );
                set_status( status::idle );
                t_lock.unlock();

                INFO( plog, "DAQ control is now active, and in the idle state" );

                f_worker_thread.join();

                INFO( plog, "DAQ control is shutting down" );
                t_lock.lock();

                if( get_status() == status::running )
                {
                    ERROR( plog, "DAQ worker quit while run was in progress" );
                    set_status( status::done );
                }
                f_daq_worker.reset();
                t_lock.unlock();

                if( t_worker_ex_ptr )
                {
                    set_status( status::error );
                    a_ex_ptr = t_worker_ex_ptr;
                }
            }
            else if( t_status == status::idle )
            {
                a_ex_ptr = std::make_exception_ptr( error() << "DAQ control status is idle in the outer execution loop!" );
                set_status( status::error );
            }
            else if( t_status == status::deactivating )
            {
                set_status( status::initialized );
            }
            else if( t_status == status::done )
            {
                INFO( plog, "Exiting DAQ control" );
                break;
            }
            else if( t_status == status::error )
            {
                if( ! a_ex_ptr ) std::make_exception_ptr( error() << "DAQ control is in an error state" );
                break;
            }
        }
        return;
    }

    void daq_control::activate()
    {
        DEBUG( plog, "Activating DAQ control" );

        if( f_canceled.load() )
        {
            throw error() << "DAQ control has been canceled";
        }

        if( get_status() != status::initialized )
        {
            throw status_error() << "DAQ control is not in the initialized state";
        }

        set_status( status::activating );
        f_condition.notify_one();

        return;
    }

    void daq_control::deactivate()
    {
        DEBUG( plog, "Deactivating DAQ" );

        if( f_canceled )
        {
            throw error() << "DAQ control has been canceled";
        }

        if( get_status() != status::idle )
        {
            throw status_error() << "Invalid state for deactivating: <" + daq_control::interpret_status( get_status() ) + ">; DAQ control must be in idle state";
        }

        std::unique_lock< std::mutex > t_lock( f_worker_mutex );
        set_status( status::deactivating );
        if( f_daq_worker )
        {
            f_daq_worker->cancel();
        }

        return;
    }

    void daq_control::start_run()
    {
        DEBUG( plog, "Preparing for run" );

        if( f_canceled )
        {
            throw error() << "daq_control has been canceled";
        }

        if( get_status() != status::idle )
        {
            throw status_error() << "DAQ control must be in the idle state to start a run";
        }

        std::unique_lock< std::mutex > t_lock( f_worker_mutex );

        if( ! f_daq_worker )
        {
            cancel();
            throw error() << "DAQ worker doesn't exist!";
        }

        INFO( plog, "Starting run" );
        try
        {
            f_daq_worker->start_run();
        }
        catch( error& e )
        {
            throw run_error() << e.what();
        }
        set_status( status::running );

        return;
    }

    void daq_control::stop_run()
    {
        INFO( plog, "Run stop requested" );

        if( get_status() != status::running ) return;

        std::unique_lock< std::mutex > t_lock( f_worker_mutex );

        if( ! f_daq_worker )
        {
            cancel();
            throw error() << "DAQ worker doesn't exist!";
        }

        try
        {
            f_daq_worker->stop_run();
        }
        catch( error& e )
        {
            throw run_error() << e.what();
        }
        set_status( status::idle );

        return;
    }

    void daq_control::notify_run_stopped()
    {
        set_status( status::idle );
    }

    void daq_control::do_cancellation()
    {
        DEBUG( plog, "Canceling DAQ control" );
        set_status( status::canceled );
        if( f_daq_worker )
        {
            f_daq_worker->cancel();
        }
        return;
    }

    bool daq_control::handle_activate_daq_control( const request_ptr_t, hub::reply_package& a_reply_pkg )
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

    bool daq_control::handle_deactivate_daq_control( const request_ptr_t, hub::reply_package& a_reply_pkg )
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

    bool daq_control::handle_start_run_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        try
        {
            start_run();
            return a_reply_pkg.send_reply( retcode_t::success, "Run started" );
        }
        catch( run_error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to start run: " ) + e.what() );
        }
    }

    bool daq_control::handle_stop_run_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        try
        {
            stop_run();
            return a_reply_pkg.send_reply( retcode_t::success, "Run stopped" );
        }
        catch( run_error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to stop run: " ) + e.what() );
        }
    }


    std::string daq_control::interpret_status( status a_status )
    {
        switch( a_status )
        {
            case status::initialized:
                return std::string( "Initialized" );
                break;
            case status::idle:
                return std::string( "Idle" );
                break;
            case status::running:
                return std::string( "Running" );
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
