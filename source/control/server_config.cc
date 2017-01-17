/*
 * server_config.cc
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
        add( "daq", t_daq_node );

        param_node* t_dev_node = new param_node();
        t_dev_node->add( "n-channels", new param_value( 1U ) );
        t_dev_node->add( "bit-depth", new param_value( 8U ) );
        t_dev_node->add( "data-type-size", new param_value( 1U ) );
        t_dev_node->add( "sample-size", new param_value( 2U ) );
        t_dev_node->add( "record-size", new param_value( 4096U ) );
        t_dev_node->add( "acq-rate", new param_value( 100U ) );
        t_dev_node->add( "v-offset", new param_value( 0.0 ) );
        t_dev_node->add( "v-range", new param_value( 0.5 ) );

        param_node* t_streams_node = new param_node();

        param_node* t_stream0_node = new param_node();
        t_stream0_node->add( "preset", new param_value( "str_1ch") );
        t_stream0_node->add( "device", t_dev_node );

        t_streams_node->add( "stream0", t_stream0_node );
    }

    server_config::~server_config()
    {
    }

} /* namespace psyllid */
