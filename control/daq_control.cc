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

using dripline::request_ptr_t;
using dripline::hub;
using dripline::retcode_t;

using std::string;

namespace psyllid
{
    LOGGER( plog, "daq_control" );

    daq_control::daq_control( std::shared_ptr< node_manager > a_mgr ) :
            cancelable(),
            f_is_running( false ),
            f_node_manager( a_mgr ),
            f_daq_worker(),
            f_worker_mutex()
    {
    }

    daq_control::~daq_control()
    {
    }

    void daq_control::do_run()
    {
        DEBUG( plog, "Preparing for run" );

        if( f_canceled )
        {
            throw error() << "daq_control has been canceled";
        }

        if( f_is_running.load() )
        {
            throw run_error() << "Run already in progress";
        }

        std::unique_lock< std::mutex > t_lock( f_worker_mutex );

        if( f_daq_worker )
        {
            throw run_error() << "Run may be in the process of starting or stopping already (daq_worker exists)";
        }

        DEBUG( plog, "Creating new DAQ worker" );
        f_daq_worker.reset( new daq_worker( ) );

        f_is_running.store( true );
        std::exception_ptr t_ex_ptr;
        midge::thread t_worker_thread;

        INFO( plog, "Starting run" );
        t_worker_thread.start( f_daq_worker.get(), &daq_worker::execute, f_node_manager, t_ex_ptr );
        t_lock.unlock();

        DEBUG( plog, "Waiting for worker thread to finish" );
        t_worker_thread.join();

        INFO( plog, "Run has ended" );
        t_lock.lock();
        f_is_running.store( false );
        f_daq_worker.reset();
        t_lock.unlock();

        if( t_ex_ptr )
        {
            std::rethrow_exception( t_ex_ptr );
        }

        return;
    }

    void daq_control::stop_run()
    {
        INFO( plog, "Run stop requested" );
        if( ! get_is_running() ) return;

        std::unique_lock< std::mutex > t_lock( f_worker_mutex );
        if( f_daq_worker )
        {
            f_daq_worker->cancel();
        }
        return;
    }

    void daq_control::do_cancellation()
    {
        DEBUG( plog, "Canceling DAQ control" );
        std::unique_lock< std::mutex > t_lock( f_worker_mutex );
        if( f_daq_worker )
        {
            f_daq_worker->cancel();
        }
        return;
    }

    bool daq_control::handle_start_run_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        try
        {
            do_run();
            return a_reply_pkg.send_reply( retcode_t::success, "Run started" );
        }
        catch( run_error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, string( "Unable to start run: " ) + e.what() );
        }
    }

    bool daq_control::handle_stop_run_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        if( get_is_running() )
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
        return a_reply_pkg.send_reply( retcode_t::success, "Run was not underway" );
    }


} /* namespace psyllid */
