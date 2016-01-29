/*
 * daq_control.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "daq_control.hh"

#include "daq_worker.hh"
#include "node_manager.hh"

namespace psyllid
{

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

        f_daq_worker.reset( new daq_worker( ) );

        t_lock.unlock();

        f_is_running.store( true );

        std::exception_ptr t_ex_ptr;

        midge::thread t_worker_thread;
        t_worker_thread.start( f_daq_worker.get(), &daq_worker::execute, f_node_manager, t_ex_ptr );

        t_worker_thread.join();

        f_is_running.store( false );

        f_daq_worker.reset();

        if( t_ex_ptr )
        {
            std::rethrow_exception( t_ex_ptr );
        }

        return;
    }

    void daq_control::do_cancellation()
    {
        std::unique_lock< std::mutex > t_lock( f_worker_mutex );
        if( f_daq_worker )
        {
            f_daq_worker->cancel();
        }
        return;
    }

} /* namespace psyllid */
