
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
    	//out_stream< 0 >() = in_stream< 0 >();
    	//out_stream< 1 >() = in_stream< 1 >();
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
        return;
    }

    void gpu_test_binding::do_dump_config( const gpu_test* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping gpu_test configuration" );
        return;

    }

    bool gpu_test_binding::do_run_command( gpu_test* a_node, const std::string& a_cmd, const scarab::param_node& ) const
    {

    	LDEBUG( plog, "Do run command gpu_test" );
    	return true;
    }
}
