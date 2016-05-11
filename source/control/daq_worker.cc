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

namespace psyllid
{
    LOGGER( plog, "daq_worker" );

    daq_worker::daq_worker() :
            f_midge_pkg(),
            f_stop_notifier()
    {
    }

    daq_worker::~daq_worker()
    {
    }

    void daq_worker::execute( std::shared_ptr< node_manager > a_node_mgr, std::exception_ptr a_ex_ptr, std::function< void() > a_notifier )
    {
        LDEBUG( plog, "DAQ worker is executing" );
        try
        {
            if( a_node_mgr->must_reset_midge() ) a_node_mgr->reset_midge();
        }
        catch( error& e )
        {
            a_ex_ptr = std::current_exception();
            return;
        }

        f_midge_pkg = a_node_mgr->get_midge();

        if( ! f_midge_pkg.have_lock() )
        {
            a_ex_ptr = std::make_exception_ptr( error() << "Could not get midge resource" );
            return;
        }

        f_stop_notifier = a_notifier;

        try
        {
            LDEBUG( plog, "Starting midge" );
            f_midge_pkg->run( a_node_mgr->get_node_run_str() );
        }
        catch( midge::error& e )
        {
            a_ex_ptr = std::current_exception();
        }

        a_notifier = nullptr;

        LDEBUG( plog, "DAQ worker finished" );

        return;
    }

    void daq_worker::start_run()
    {
        if( ! f_midge_pkg.have_lock() )
        {
            throw error() << "Do not have midge resource";
        }
        f_midge_pkg->instruct( midge::instruction::resume );
        return;
    }

    void daq_worker::stop_run()
    {
        if( ! f_midge_pkg.have_lock() )
        {
            throw error() << "Do not have midge resource";
        }
        f_midge_pkg->instruct( midge::instruction::pause );
        if( f_stop_notifier ) f_stop_notifier();
        return;
    }


    void daq_worker::do_cancellation()
    {
        LDEBUG( plog, "Canceling DAQ worker" );
        f_midge_pkg->cancel();
        return;
    }


} /* namespace psyllid */
