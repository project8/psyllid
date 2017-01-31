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

    void node_builder::configure( const scarab::param_node& a_config )
    {
        f_config.merge( a_config );
        return;
    }

    void node_builder::replace_config( const scarab::param_node& a_config )
    {
        f_config.clear();
        f_config.merge( a_config );
        return;
    }

    void node_builder::dump_config( scarab::param_node& a_config )
    {
        a_config.clear();
        a_config.merge( f_config );
        return;
    }

} /* namespace psyllid */
