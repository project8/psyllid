/*
 * test_event_builder.cc
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 *
 *  Trigger ids:          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ,16, 17, 18, 19
 *  Sequence of triggers: 0, 0, 0, 1, 1, 1, 0, 1, 1, 0,  0,  1,  1,  1,  0,  1,  0,  0,  0,  1
 *
 *  Pretrigger: 2
 *  Skip tolerance: 1
 *
 *  Triggered events should be: 1-8, 9-15, 17-19
 */


#include "event_builder.hh"
#include "midge.hh"
#include "producer.hh"

#include "logger.hh"

#include <array>

using std::array;

using midge::stream;

LOGGER( plog, "test_event_builder" );

namespace psyllid
{
    class test_producer :
            public midge::_producer< test_producer, typelist_1( trigger_flag ) >
    {
        public:
            test_producer() :
                f_length( 10 )
            {}

            virtual ~test_producer() {}

        public:
            accessible( uint64_t, length );

        public:
            void initialize()
            {
                out_buffer< 0 >().initialize( f_length );
                return;
            }

            void execute()
            {
                // this line produces a warning in clang and some versions of gcc; it's a known bug in the compilers
                array< bool, 20 > t_flags = { 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1 };

                out_stream< 0 >().set( stream::s_start );

                for( uint64_t t_id = 0; t_id < t_flags.size(); ++t_id )
                {
                    trigger_flag* t_flag_data = out_stream< 0 >().data();

                    t_flag_data->set_id( t_id );
                    t_flag_data->set_flag( t_flags[ t_id ] );

                    DEBUG( plog, "Sending: " << t_id << ", " << t_flags[t_id] );

                    out_stream< 0 >().set( stream::s_run );
                }

                out_stream< 0 >().set( stream::s_stop );
                out_stream< 0 >().set( stream::s_exit );

                return;
            }

            void finalize()
            {
                out_buffer< 0 >().finalize();
                return;
            }


    };
}


using namespace psyllid;
using namespace midge;

int main()
{
    INFO( plog, "Preparing" );

    ::midge::midge* t_root = new ::midge::midge();

    test_producer* t_test_prod = new test_producer();
    t_test_prod->set_name( "tp" );
    t_root->add( t_test_prod );

    event_builder* t_event_builder = new event_builder();
    t_event_builder->set_name( "eb" );
    t_event_builder->set_pretrigger( 2 );
    t_event_builder->set_skip_tolerance( 1 );
    t_root->add( t_event_builder );

    t_root->join( "tp.out_0:eb.in_0" );

    INFO( plog, "Expecting the following events: 1-8, 9-15, 17-19" );

    INFO( plog, "Starting execution" );

    t_root->run( "tp:eb" );

    INFO( plog, "All done!" );

    delete t_root;

    return 0;
}

