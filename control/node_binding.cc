/*
 * node_binding.cc
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#include "node_binding.hh"

namespace psyllid
{

    node_binding::node_binding( const std::string& a_name ) :
            f_name( a_name ),
            f_config()
    {
    }

    node_binding::~node_binding()
    {
    }

    void node_binding::configure( const scarab::param_node& a_node )
    {
        f_config.merge( a_node );
        return;
    }

    void node_binding::replace_config( const scarab::param_node& a_node )
    {
        f_config.clear();
        f_config.merge( a_node );
        return;
    }

    void node_binding::dump_config( scarab::param_node& a_node )
    {
        a_node.clear();
        a_node.merge( f_config );
        return;
    }

} /* namespace psyllid */
