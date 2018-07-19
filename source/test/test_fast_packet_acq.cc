/*
 * test_fast_packet_acq.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 *
 *  Usage: > test_fast_packet_acq [options]
 *
 *  Options:
 *    - interface -- (string) network interface to listen on; defaults to "eth1"
 */


#include "tf_roach_receiver.hh"
#include "fpa_factory.hh"
#include "psyllid_error.hh"
#include "midge_error.hh"

#include "configurator.hh"
#include "logger.hh"
#include "param.hh"


using namespace psyllid;

LOGGER( plog, "test_fast_packet_acq" );

int main( int argc, char** argv )
{
    try
    {
        scarab::param_node t_default_config;
        t_default_config.add( "interface", scarab::param_value( "eth1" ) );

        scarab::configurator t_configurator( argc, argv, &t_default_config );

        std::string t_interface( t_configurator.get< std::string >( "interface" ) );

        // setup the server config
        scarab::param_node t_server_config = scarab::param_node();
        t_server_config.add( "type", scarab::param_value( "fpa" ) );
        t_server_config.add( "interface", scarab::param_value( t_interface ) );

        LINFO( plog, "Creating and configuring receiver" );

        tf_roach_receiver t_receiver;
        t_receiver.set_name( "rec" );
        //t_receiver.node_ptr( &t_fpa, t_interface );
        t_receiver.set_server_config( t_server_config );

        LINFO( plog, "Initializing receiver & FPA" );

        t_receiver.initialize();

        LINFO( plog, "Running receiver" );

        t_receiver.execute();

        LINFO( plog, "Finalizing receiver and FPA" );

        t_receiver.finalize();

        return 0;
    }
    catch( psyllid::error& e )
    {
        LERROR( plog, "Psyllid exception caught: " << e.what() );
        return -1;
    }
    catch( midge::error& e )
    {
        LERROR( plog, "Midge exception caught: " << e.what() );
        return -1;
    }

}
