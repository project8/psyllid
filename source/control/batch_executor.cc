/*
 * batch_executor.cc
 *
 * Created on: April 12, 2018
 *     Author: laroque
 */

#include "batch_executor.hh"

namespace psyllid
{

    batch_executor::batch_executor() :
        f_actions_node()
    {
    }

    batch_executor::batch_executor( const scarab::param_node& a_master_config ) :
        f_actions_node( *a_master_config.node_at( "batch-actions" ) )
    {
    }

    batch_executor::~batch_executor()
    {
    }

    void batch_executor::execute()
    {
    }

} /* namespace psyllid */
