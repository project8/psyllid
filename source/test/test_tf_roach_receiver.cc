/*
 * test_tf_roach_receiver.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 *
 *  Usage: > test_tf_roach_receiver [options]
 *
 *  Parameters:
 *    - type: (string) server type name; options are "socket" (default) and "fpa"
 *    - interface: (string) network interface name; this is only needed if using the FPA server; default is "eth1"
 */


#include "psyllid_error.hh"
#include "tf_roach_receiver.hh"

//#ifdef __linux__
//#include "fast_packet_acq.hh"
//#endif

#include "configurator.hh"
#include "logger.hh"
#include "param.hh"


using namespace psyllid;

LOGGER( plog, "test_tf_roach_receiver" );

int main( int argc, char** argv )
{
    try
    {
        scarab::param_node t_default_config;
        t_default_config.add( "type", new scarab::param_value( "socket" ) );
        t_default_config.add( "interface", new scarab::param_value( "eth1" ) );

        scarab::configurator t_configurator( argc, argv, &t_default_config );

/*
#ifdef __linux__
        fast_packet_acq* t_fpa = nullptr;
        if( t_server_type == "fpa" )
        {
            LINFO( plog, "Creating fast-packet acquisition" );
            t_fpa = new fast_packet_acq( t_configurator.get< std::string >( "interface" ) );
            t_fpa->initialize();
        }
#endif
*/
        LINFO( plog, "Creating receiver" );

        tf_roach_receiver t_receiver;
        t_receiver.set_server_config( new scarab::param_node( t_configurator.config() ) );

        LINFO( plog, "Initializing receiver" );

        t_receiver.initialize();

        LINFO( plog, "Running receiver" );

        t_receiver.execute();

        LINFO( plog, "Finalizing receiver" );

        t_receiver.finalize();
/*
#ifdef __linux__
        delete t_fpa;
#endif
*/
        return 0;
    }
    catch( std::exception& e )
    {
        LERROR( plog, "Exception caught: " << e.what() );
        return -1;
    }

}
