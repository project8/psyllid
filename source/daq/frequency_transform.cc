
/*
 * frequency_transform.cc
 *
 *  Created on: March 28, 2018
 *      Author: laroque
 */

#include "frequency_transform.hh"

//#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <thread>
//#include <memory>
#include <sys/types.h> // for ssize_t

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( frequency_transform, "frequency-transform", frequency_transform_binding );

    LOGGER( plog, "frequency_transform" );

    frequency_transform::frequency_transform() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            //f_udp_buffer_size( sizeof( roach_packet ) ),
            //f_time_sync_tol( 2 ),
            f_start_paused( true ),
            //f_skip_after_stop( 0 ),
            //f_exe_func_mutex(),
            //f_break_exe_func( false ),
            f_paused( true )
            //TODO we maybe want these?
            //f_time_session_pkt_counter( 0 ),
            //f_freq_session_pkt_counter( 0 )
    {
    }

    frequency_transform::~frequency_transform()
    {
    }

    void frequency_transform::initialize()
    {
        out_buffer< 0 >().initialize( f_time_length );
        out_buffer< 1 >().initialize( f_freq_length );
        return;
    }

    void frequency_transform::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the frequency transformer" );

            //exe_func_context t_ctx;

            //t_ctx.f_midge = a_midge;

            //t_ctx.f_in_command = stream::s_none;

            //t_ctx.f_memory_block = nullptr;
            //t_ctx.f_time_data = nullptr;
            //t_ctx.f_freq_data = nullptr;

            //t_ctx.f_buffer_ptr.reset( new char[ f_udp_buffer_size ] );

            f_paused = f_start_paused;

            //uint64_t t_time_batch_pkt = 0;
            //uint64_t t_freq_batch_pkt = 0;

            //t_ctx.f_pkt_size = 0;

            if( ! f_start_paused )
            {
                LDEBUG( plog, "FREQUENCY TRANSFORM starting unpaused" );
                out_stream< 0 >().data()->set_pkt_in_session( 0 );
                out_stream< 1 >().data()->set_pkt_in_session( 0 );
                if( ! out_stream< 0 >().set( stream::s_start ) ) return;
                if( ! out_stream< 1 >().set( stream::s_start ) ) return;
                //TODO need this?
                //f_time_session_pkt_counter = 0;
                //f_freq_session_pkt_counter = 0;
            }

            try
            {
                LPROG( plog, "Starting main loop (frequency transform)" );
                //f_break_exe_func.store( true );
                //while( f_break_exe_func.load() )
                //{
                //    f_break_exe_func.store( false );
                //    f_exe_func_mutex.lock();
                //    if( ! (this->*f_exe_func)( t_ctx ) ) return;
                //}
            }
            catch( error& e )
            {
                throw;
            }

            LINFO( plog, "FREQUENCY TRANSFORM is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output streams" );
            bool t_stop_ok = out_stream< 0 >().set( stream::s_stop ) && out_stream< 1 >().set( stream::s_stop );
            if( ! t_stop_ok ) return;

            LDEBUG( plog, "Exiting output streams" );
            if( ! out_stream< 0 >().set( stream::s_exit ) ) return;
            out_stream< 1 >().set( stream::s_exit );

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }

        return;
    }

    void frequency_transform::finalize()
    {
        out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        return;
    }


    // frequency_transform_binding methods
    frequency_transform_binding::frequency_transform_binding() :
            _node_binding< frequency_transform, frequency_transform_binding >()
    {
    }

    frequency_transform_binding::~frequency_transform_binding()
    {
    }

    void frequency_transform_binding::do_apply_config( frequency_transform* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring frequency_transform with:\n" << a_config );
        a_node->set_time_length( a_config.get_value( "time-length", a_node->get_time_length() ) );
        a_node->set_freq_length( a_config.get_value( "freq-length", a_node->get_freq_length() ) );
        a_node->set_start_paused( a_config.get_value( "start-paused", a_node->get_start_paused() ) );
        return;
    }

    void frequency_transform_binding::do_dump_config( const frequency_transform* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping frequency_transform configuration" );
        a_config.add( "time-length", new scarab::param_value( a_node->get_time_length() ) );
        a_config.add( "freq-length", new scarab::param_value( a_node->get_freq_length() ) );
        a_config.add( "start-paused", new scarab::param_value( a_node->get_start_paused() ) );
        return;
    }

    //bool frequency_transform_binding::do_run_command( frequency_transform* a_node, const std::string& a_cmd, const scarab::param_node& ) const
    //{
    //    if( a_cmd == "freq-only" )
    //    {
    //        a_node->switch_to_freq_only();
    //        return true;
    //    }
    //    else if( a_cmd == "time-and-freq" )
    //    {
    //        a_node->switch_to_time_and_freq();
    //        return true;
    //    }
    //    else
    //    {
    //        LWARN( plog, "Unrecognized command: <" << a_cmd << ">" );
    //        return false;
    //    }
    //}

} /* namespace psyllid */
