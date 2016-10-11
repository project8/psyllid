/*
 * daq_worker.cc
 *
 *  Created on: Jan 24, 2016
 *      Author: nsoblath
 */

#include "daq_worker.hh"

#include "node_manager.hh"
#include "psyllid_error.hh"

#include "diptera.hh"
#include "midge_error.hh"

#include "logger.hh"

#include <chrono>
#include <future>

namespace psyllid
{
    LOGGER( plog, "daq_worker" );

    daq_worker::daq_worker() :
            scarab::cancelable(),
            f_midge_pkg(),
            f_run_in_progress( false ),
            f_error_state( false ),
            f_run_return(),
            f_run_stopper(),
            f_stop_notifier()
    {
    }

    daq_worker::~daq_worker()
    {
    }

    void daq_worker::execute( std::shared_ptr< node_manager > a_node_mgr, std::exception_ptr a_ex_ptr, std::function< void( bool ) > a_notifier )
    {
        LDEBUG( plog, "DAQ worker is executing" );
        try
        {
            if( a_node_mgr->must_reset_midge() ) a_node_mgr->reset_midge();
        }
        catch( error& e )
        {
            LERROR( plog, "Exception caught while resetting midge" );
            a_ex_ptr = std::current_exception();
            return;
        }

        LDEBUG( plog, "Acquiring midge package" );
        f_midge_pkg = a_node_mgr->get_midge();

        if( ! f_midge_pkg.have_lock() )
        {
            a_ex_ptr = std::make_exception_ptr( error() << "Could not get midge resource" );
            return;
        }

        f_stop_notifier = a_notifier;

        try
        {
            std::string t_run_string( a_node_mgr->get_node_run_str() );
            LDEBUG( plog, "Starting midge with run string <" << t_run_string << ">" );
            f_midge_pkg->run( t_run_string );
        }
        catch( std::exception& e )
        {
            a_node_mgr->return_midge( std::move( f_midge_pkg ) );
            LERROR( plog, "An exception was thrown while running midge: " << e.what() );
            a_ex_ptr = std::current_exception();
        }

        a_node_mgr->return_midge( std::move( f_midge_pkg ) );

        if( f_run_in_progress.load() )
        {
            LERROR( plog, "Midge exited abnormally" );
            f_error_state.store( true );
            f_run_stopper.notify_one();
        }

        f_stop_notifier = nullptr;

        LDEBUG( plog, "DAQ worker finished" );

        return;
    }

    void daq_worker::start_run( unsigned a_duration )
    {
        if( ! f_midge_pkg.have_lock() )
        {
            throw error() << "Do not have midge resource";
        }
        if( f_run_in_progress.load() )
        {
            throw error() << "A run is already in progress";
        }
        LDEBUG( plog, "Launching asynchronous do_run" );
        f_run_return = std::async( std::launch::async, &daq_worker::do_run, this, a_duration );
        return;
    }

    void daq_worker::do_run( unsigned a_duration )
    {
    	LINFO( plog, "Run is commencing" );
        std::unique_lock< std::mutex > t_run_stop_lock( f_run_stop_mutex );
        f_run_in_progress.store( true );
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
        f_run_in_progress.store( false );
        if( f_stop_notifier )
        {
            if( f_error_state.load() ) f_stop_notifier( true );
            else f_stop_notifier( false );
        }
        LINFO( plog, "Run has stopped" );
        return;
    }

    void daq_worker::stop_run()
    {
        if( ! f_midge_pkg.have_lock() )
        {
            throw error() << "Do not have midge resource; worker is not currently running";
        }
        std::unique_lock< std::mutex > t_run_stop_lock( f_run_stop_mutex );
        if( ! f_run_in_progress.load() ) return;
        f_run_stopper.notify_all();
        return;
    }


    void daq_worker::do_cancellation()
    {
        LDEBUG( plog, "Canceling DAQ worker" );
        if( f_midge_pkg.have_lock() ) f_midge_pkg->cancel();
        return;
    }


} /* namespace psyllid */
