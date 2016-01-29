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

namespace psyllid
{

    daq_worker::daq_worker() :
            f_midge()
    {
    }

    daq_worker::~daq_worker()
    {
    }

    void daq_worker::execute( std::shared_ptr< node_manager > a_node_mgr, std::exception_ptr a_ex_ptr )
    {
        try
        {
            if( a_node_mgr->must_reset_midge() ) a_node_mgr->reset_midge();
        }
        catch( error& e )
        {
            a_ex_ptr = std::current_exception();
            return;
        }

        f_midge = a_node_mgr->get_midge();

        try
        {
            f_midge->run( a_node_mgr->get_node_run_str() );
        }
        catch( midge::error& e )
        {
            a_ex_ptr = std::current_exception();
            return;
        }

        return;
    }

    void daq_worker::do_cancellation()
    {
        f_midge->cancel();
        return;
    }


} /* namespace psyllid */
