/*
 * single_value_daq.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#include "diptera.hh"
//#include "egg_writer.hh"
#include "event_builder.hh"
#include "frequency_mask_trigger.hh"
#include "udp_receiver.hh"

#include "logger.hh"

#include <memory>

using std::unique_ptr;

using namespace midge;
using namespace psyllid;

LOGGER( plog, "roach_daq_1chan" );

int main()
{
    try
    {
        //unique_ptr< ::midge::diptera > t_root;
        diptera* t_root = new diptera();

        udp_receiver* t_udpr = new udp_receiver();
        t_udpr->set_name( "udpr" );
        t_udpr->set_time_length( 100 );
        t_root->add( t_udpr );

        frequency_mask_trigger* t_fmt = new frequency_mask_trigger();
        t_fmt->set_name( "fmt" );
        t_fmt->set_threshold_ampl_snr( 2.0 );
        t_root->add( t_fmt );

        event_builder* t_eb = new event_builder();
        t_eb->set_name( "eb" );
        t_eb->set_pretrigger( 1 );
        t_eb->set_skip_tolerance( 0 );
        t_root->add( t_eb );
/*
        egg_writer* t_ew = new egg_writer();
        t_ew->set_name( "ew" );
        // set parameters
        t_root->add( t_ew );
*/
        //t_root->join( "udpr.out_0:ew.in_0" );
        t_root->join( "udpr.out_1:fmt.in_0" );
        t_root->join( "fmt.out_0:eb.in_0" );
        //t_root->join( "eb.out_0:ew.in_1" );

        t_root->run( "udpr:fmt:eb"/*:ew"*/ );

        delete t_root;
    }
    catch( std::exception& e )
    {
        ERROR( plog, "Exception caught: " << e.what() );
    }

    return 0;
}


