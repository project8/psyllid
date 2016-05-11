/*
 * psyllid_config.cc
 *
 *  Created on: Nov 4, 2013
 *      Author: nsoblath
 */

#include "server_config.hh"

#include<string>

using std::string;

using scarab::param_node;
using scarab::param_value;

namespace psyllid
{

    server_config::server_config()
    {
        // default server configuration

        param_node* t_amqp_node = new param_node();
        t_amqp_node->add( "broker-port", new param_value( 5672 ) );
        t_amqp_node->add( "broker", new param_value( "localhost" ) );
        t_amqp_node->add( "request-exchange", new param_value( "requests" ) );
        t_amqp_node->add( "alert-exchange", new param_value( "alerts" ) );
        t_amqp_node->add( "queue", new param_value( "psyllid" ) );
        t_amqp_node->add( "listen-timeout-ms", new param_value( 100 ) );
        add( "amqp", t_amqp_node );

        param_node* t_daq_node = new param_node();
        t_daq_node->add( "activate-at-startup", new param_value( true ) );
        t_daq_node->add( "preset", new param_value( "roach" ) );
        param_node* t_monarch_node = new param_node();
        t_monarch_node->add( "max-file-size-mb", new param_value( 1000 ) );
        t_daq_node->add( "monarch", t_monarch_node );
        add( "daq", t_daq_node );

    }

    server_config::~server_config()
    {
    }

} /* namespace psyllid */
