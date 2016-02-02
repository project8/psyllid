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
            f_midge_pkg()
    {
    }

    daq_worker::~daq_worker()
    {
    }

    void daq_worker::execute( std::shared_ptr< node_manager > a_node_mgr, std::exception_ptr a_ex_ptr )
    {
        DEBUG( plog, "DAQ worker is executing" );
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

        try
        {
            DEBUG( plog, "Starting midge" );
            f_midge_pkg->run( a_node_mgr->get_node_run_str() );
        }
        catch( midge::error& e )
        {
            a_ex_ptr = std::current_exception();
            return;
        }

        DEBUG( plog, "DAQ worker finished" );

        return;
    }

    void daq_worker::do_cancellation()
    {
        DEBUG( plog, "Canceling DAQ worker" );
        f_midge_pkg->cancel();
        return;
    }


} /* namespace psyllid */
