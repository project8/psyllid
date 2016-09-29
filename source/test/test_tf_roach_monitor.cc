/*
 * test_tf_roach_monitor.cc
 *
 *  Created on: Sep 23, 2016
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 *
 *  Usage: > test_tf_roach_monitor [options]
 *
 *  Options:
 *    - type: server type name; options are "socket" (default) and "fpa"
 *    - interface: network interface name; only used if using "fpa" server type; default is "eth1"
 */


#include "psyllid_error.hh"
#include "terminator.hh"
#include "tf_roach_monitor.hh"
#include "tf_roach_receiver.hh"

#include "diptera.hh"

#include "configurator.hh"
#include "logger.hh"
#include "param.hh"

#include <signal.h>

using namespace psyllid;


LOGGER( plog, "test_tf_roach_monitor" );

scarab::cancelable* f_cancelable = nullptr;

void cancel( int )
{
    LINFO( plog, "Attempting to cancel" );
    if( f_cancelable != nullptr ) f_cancelable->cancel();
    return;
}

int main( int argc, char** argv )
{
    try
    {
        scarab::param_node t_default_config;
        t_default_config.add( "type", new scarab::param_value( "socket" ) );
        t_default_config.add( "interface", new scarab::param_value( "eth1" ) );

        scarab::configurator t_configurator( argc, argv, &t_default_config );

        std::string t_server_type( t_configurator.get< std::string >( "type" ) );
        scarab::param_node* t_server_config = new scarab::param_node(  );
        t_server_config->add( "type", new scarab::param_value( t_server_type ) );
        if( t_server_type == "fpa" )
        {
            t_server_config->add( "interface", new scarab::param_value( t_configurator.get< std::string >( "interface" ) ) );
        }

        LINFO( plog, "Creating and configuring nodes" );

        midge::diptera* t_root = new midge::diptera();

        tf_roach_receiver* t_rec = new tf_roach_receiver();
        t_rec->set_name( "rec" );
        t_rec->set_time_length( 10 );
        t_rec->set_server_config( t_server_config );
        t_root->add( t_rec );

        roach_time_monitor* t_tmon = new roach_time_monitor();
        t_tmon->set_name( "tmon" );
        t_root->add( t_tmon );

        terminator_freq_data* t_term = new terminator_freq_data();
        t_term->set_name( "term" );
        t_root->add( t_term );

        LINFO( plog, "Connecting nodes" );

        t_root->join( "rec.out_0:tmon.in_0" );
        t_root->join( "rec.out_1:term.in_0" );

        LINFO( plog, "Exit with ctrl-c" );

        // set up signal handling for canceling with ctrl-c
        f_cancelable = t_rec;
        signal( SIGINT, cancel );

        LINFO( plog, "Executing" );

        t_root->run( "udpr:fmt:eb:ew" );

        LINFO( plog, "Execution complete" );

        // un-setup signal handling
        f_cancelable = nullptr;

        delete t_root;

        return 0;
    }
    catch( std::exception& e )
    {
        LERROR( plog, "Exception caught: " << e.what() );
        return -1;
    }

}


