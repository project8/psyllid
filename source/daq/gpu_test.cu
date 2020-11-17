
/*
 * gpu_test.cu
 *
 *  Created on: Nov 10, 2020
 *      Author: F. Thomas
 */

#include "gpu_test.cuh"
#include <stdio.h>


__global__ void hello_gpu()
{
	int idx = blockDim.x*blockIdx.x+threadIdx.x;

	printf("Hello from GPU thread %d\n", idx);
}

namespace psyllid{

	REGISTER_NODE_AND_BUILDER( gpu_test, "gpu-test", gpu_test_binding );

	LOGGER( plog, "gpu_test" );

	gpu_test::gpu_test() :
		f_time_length( 10 ),
		f_freq_length( 10 )
	{}

	gpu_test::~gpu_test() {}

    void gpu_test::initialize()
    {
        out_buffer< 0 >().initialize( f_time_length );
        out_buffer< 1 >().initialize( f_freq_length );
        return;
    }

    void gpu_test::execute( midge::diptera* a_midge)
    {
    	out_stream< 0 >() = in_stream< 0 >();
    	out_stream< 1 >() = in_stream< 1 >();
    	LDEBUG( plog, "Executing gpu_test" );
    	hello_gpu<<<1, 4>>>();
    	cudaDeviceSynchronize();

    }

    void gpu_test::finalize()
    {
        //out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        return;
    }

    gpu_test_binding::gpu_test_binding() :
            _node_binding< gpu_test, gpu_test_binding >()
    {
    }

    gpu_test_binding::~gpu_test_binding()
    {
    }

    void gpu_test_binding::do_apply_config( gpu_test* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring gpu_test with:\n" << a_config );
        /*a_node->set_time_length( a_config.get_value( "time-length", a_node->get_time_length() ) );
        a_node->set_freq_length( a_config.get_value( "freq-length", a_node->get_freq_length() ) );
        a_node->set_udp_buffer_size( a_config.get_value( "udp-buffer-size", a_node->get_udp_buffer_size() ) );
        a_node->set_time_sync_tol( a_config.get_value( "time-sync-tol", a_node->get_time_sync_tol() ) );
        a_node->set_start_paused( a_config.get_value( "start-paused", a_node->get_start_paused() ) );
        a_node->set_force_time_first( a_config.get_value( "force-time-first", a_node->get_force_time_first() ) );*/
        return;
    }

    void gpu_test_binding::do_dump_config( const gpu_test* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping gpu_test configuration" );
        /*a_config.add( "time-length", scarab::param_value( a_node->get_time_length() ) );
        a_config.add( "freq-length", scarab::param_value( a_node->get_freq_length() ) );
        a_config.add( "udp-buffer-size", scarab::param_value( a_node->get_udp_buffer_size() ) );
        a_config.add( "time-sync-tol", scarab::param_value( a_node->get_time_sync_tol() ) );
        a_config.add( "start-paused", scarab::param_value( a_node->get_start_paused() ) );
        a_config.add( "force-time-first", scarab::param_value( a_node->get_force_time_first() ) );*/
        return;

    }

    bool gpu_test_binding::do_run_command( gpu_test* a_node, const std::string& a_cmd, const scarab::param_node& ) const
    {
        /*if( a_cmd == "freq-only" )
        {
            a_node->switch_to_freq_only();
            return true;
        }
        else if( a_cmd == "time-and-freq" )
        {
            a_node->switch_to_time_and_freq();
            return true;
        }
        else
        {
            LWARN( plog, "Unrecognized command: <" << a_cmd << ">" );
            return false;
        }*/

    	LDEBUG( plog, "Do run command gpu_test" );
    	return true;
    }
}
