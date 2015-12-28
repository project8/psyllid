/*
 * test_udp_receiver.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 *
 *  Suggested UDP client: roach_simulator.go
 */


#include "error.hh"
#include "psyllidmsg.hh"
#include "udp_receiver.hh"



using namespace midge;
using namespace psyllid;

int main()
{
    try
    {
        pmsg( s_normal ) << "Creating receiver" << eom;

        udp_receiver t_receiver;

        pmsg( s_normal ) << "Initializing receiver" << eom;

        t_receiver.initialize();

        pmsg( s_normal ) << "Running receiver" << eom;

        t_receiver.execute();

        pmsg( s_normal ) << "Finalizing receiver" << eom;

        t_receiver.finalize();

        return 0;
    }
    catch( error& e )
    {
        pmsg( s_error ) << "Exception caught: " << e.what() << eom;
        return -1;
    }

}
