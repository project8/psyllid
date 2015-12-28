/*
 * single_value_daq.cc
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#include "single_value_trigger.hh"
#include "udp_receiver.hh"

#include "midge.hh"

#include <memory>
using std::unique_ptr;

using namespace midge;
using namespace psyllid;

int main()
{
    try
    {
        messages* t_messages = messages::get_instance();
        t_messages->set_terminal_severity( s_debug );
        t_messages->set_terminal_stream( &cout );

        //unique_ptr< ::midge::midge > t_root;
        ::midge::midge* t_root = new ::midge::midge();

        udp_receiver* t_udpr = new udp_receiver();
        t_udpr->set_name( "udpr" );
        // set parameters
        t_root->add( t_udpr );

        single_value_trigger* t_svt = new single_value_trigger();
        t_svt->set_name( "svt" );
        // set parameters
        t_root->add( t_svt );

        t_root->join( "udpr.out_1:svt.in_0" );

        t_root->run( "udpr:svt" );

        delete t_root;
    }
    catch( std::exception& e )
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }

    return 0;
}


