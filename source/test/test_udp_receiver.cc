/*
 * test_udp_receiver.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 */


#include "midge_error.hh"
#include "udp_receiver.hh"

#include "logger.hh"


using namespace midge;
using namespace psyllid;

LOGGER( plog, "test_udp_receiver" );

int main()
{
    try
    {
        LINFO( plog, "Creating receiver" );

        udp_receiver t_receiver;

        LINFO( plog, "Initializing receiver" );

        t_receiver.initialize();

        LINFO( plog, "Running receiver" );

        t_receiver.execute();

        LINFO( plog, "Finalizing receiver" );

        t_receiver.finalize();

        return 0;
    }
    catch( error& e )
    {
        LERROR( plog, "Exception caught: " << e.what() );
        return -1;
    }

}
