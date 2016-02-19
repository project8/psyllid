/*
 * node_builder.cc
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#include "node_builder.hh"

namespace psyllid
{

    node_builder::node_builder() :
            f_name(),
            f_config()
    {
    }

    node_builder::~node_builder()
    {
    }

    void node_builder::configure( const scarab::param_node& a_node )
    {
        f_config.merge( a_node );
        return;
    }

    void node_builder::replace_config( const scarab::param_node& a_node )
    {
        f_config.clear();
        f_config.merge( a_node );
        return;
    }

    void node_builder::dump_config( scarab::param_node& a_node )
    {
        a_node.clear();
        a_node.merge( f_config );
        return;
    }

} /* namespace psyllid */
