/*
 * test_fast_packet_acq.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 */


#include "fast_packet_acq.hh"
#include "psyllid_error.hh"
#include "udp_receiver.hh"

#include "midge_error.hh"

#include "logger.hh"
#include "param.hh"


using namespace midge;
using namespace psyllid;

LOGGER( plog, "test_udp_receiver" );

int main()
{
    try
    {
        scarab::param_node* t_server_config = new scarab::param_node();
        t_server_config->add( "type", new scarab::param_value( "fpa" ) );
        t_server_config->add( "fpa", new scarab::param_value( "eth1-fpa" ) );

        LINFO( plog, "Creating and configuring FPA" );

        fast_packet_acq t_fpa;
        t_fpa.net_interface() = "eth1";

        LINFO( plog, "Creating and configuring receiver" );

        udp_receiver t_receiver;
        t_receiver.node_ptr( &t_fpa, t_server_config->get_value( "fpa" ) );
        t_receiver.set_server_config( t_server_config );

        LINFO( plog, "Initializing receiver & FPA" );

        t_fpa.initialize();
        t_receiver.initialize();

        LINFO( plog, "Running receiver" );

        t_receiver.execute();

        LINFO( plog, "Running FPA" );

        t_fpa.execute();

        LINFO( plog, "Finalizing receiver and FPA" );

        t_receiver.finalize();
        t_fpa.finalize();

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
