/*
 * gpu_test.cuh
 *
 *  Created on: Nov 10, 2020
 *      Author: F. Thomas
 */

#ifndef GPU_TEST_CUH
#define GPU_TEST_CUH

#include "freq_data.hh"
#include "memory_block.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "transformer.hh"

namespace scarab
{
    class param_node;
}

namespace psyllid {

	class gpu_test : public midge::_transformer< midge::type_list< time_data, freq_data >, midge::type_list< time_data, freq_data > >

	{

	public:

		gpu_test();
		virtual ~gpu_test();

        virtual void initialize();
        virtual void execute( midge::diptera* a_midge = nullptr );
        virtual void finalize();

	private:
        mv_accessible( uint64_t, time_length );
        mv_accessible( uint64_t, freq_length );

	};

    class gpu_test_binding : public _node_binding< gpu_test, gpu_test_binding >
    {
        public:
            gpu_test_binding();
            virtual ~gpu_test_binding();

        private:
            virtual void do_apply_config( gpu_test* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const gpu_test* a_node, scarab::param_node& a_config ) const;

            virtual bool do_run_command( gpu_test* a_node, const std::string& a_cmd, const scarab::param_node& ) const;
    };

}

#endif /*GPU_TEST_CUH*/
