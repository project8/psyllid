/*
 * test_udp_receiver.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 */


#include "tf_roach_receiver.hh"

#include "midge_error.hh"

#include "logger.hh"


using namespace midge;
using namespace psyllid;

LOGGER( plog, "test_tf_roach_receiver" );

int main()
{
    try
    {
        LINFO( plog, "Creating receiver" );

        tf_roach_receiver t_receiver;

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
