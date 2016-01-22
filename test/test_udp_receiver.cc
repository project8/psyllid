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
        INFO( plog, "Creating receiver" );

        udp_receiver t_receiver;

        INFO( plog, "Initializing receiver" );

        t_receiver.initialize();

        INFO( plog, "Running receiver" );

        t_receiver.execute();

        INFO( plog, "Finalizing receiver" );

        t_receiver.finalize();

        return 0;
    }
    catch( error& e )
    {
        ERROR( plog, "Exception caught: " << e.what() );
        return -1;
    }

}
