
/*
 * udp_receiver.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "tf_roach_receiver.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

#include <thread>
#include <memory>
#include <sys/types.h> // for ssize_t

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( tf_roach_receiver, "tf-roach-receiver", tf_roach_receiver_binding );

    LOGGER( plog, "tf_roach_receiver" );

    tf_roach_receiver::tf_roach_receiver() :
            f_time_length( 10 ),
            f_freq_length( 10 ),
            f_udp_buffer_size( sizeof( roach_packet ) ),
            f_time_sync_tol( 2 ),
            f_start_paused( true ),
            f_force_time_first( false ),
            f_skip_after_stop( 0 ),
            f_exe_func( &tf_roach_receiver::exe_time_and_freq ),
            f_exe_func_mutex(),
            f_break_exe_func( false ),
            f_paused( true ),
            f_time_session_pkt_counter( 0 ),
            f_freq_session_pkt_counter( 0 )
    {
    }

    tf_roach_receiver::~tf_roach_receiver()
    {
    }

    void tf_roach_receiver::switch_to_freq_only()
    {
        LDEBUG( plog, "Requesting switch to frequency-only mode" );
        f_exe_func_mutex.lock();
        if( f_exe_func != &tf_roach_receiver::exe_freq_only )
        {
            f_break_exe_func.store( true );
            f_exe_func = &tf_roach_receiver::exe_freq_only;
        }
        f_exe_func_mutex.unlock();
        return;
    }

    void tf_roach_receiver::switch_to_time_and_freq()
    {
        LDEBUG( plog, "Requesting switch to time-and-frequency mode" );
        f_exe_func_mutex.lock();
        if( f_exe_func != &tf_roach_receiver::exe_time_and_freq )
        {
            f_break_exe_func.store( true );
            f_exe_func = &tf_roach_receiver::exe_time_and_freq;
        }
        f_exe_func_mutex.unlock();
        return;
    }

    void tf_roach_receiver::initialize()
    {
        out_buffer< 0 >().initialize( f_time_length );
        out_buffer< 1 >().initialize( f_freq_length );
        return;
    }

    void tf_roach_receiver::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the TF ROACH receiver" );

            exe_func_context t_ctx;

            t_ctx.f_midge = a_midge;

            t_ctx.f_in_command = stream::s_none;

            t_ctx.f_memory_block = nullptr;
            t_ctx.f_time_data = nullptr;
            t_ctx.f_freq_data = nullptr;

            //LDEBUG( plog, "Server is listening" );

            t_ctx.f_buffer_ptr.reset( new char[ f_udp_buffer_size ] );

            // start out as configured (default is paused)
            //out_stream< 0 >().set( stream::s_start );
            //out_stream< 1 >().set( stream::s_start );
            f_paused = f_start_paused;

            //uint64_t t_time_batch_pkt = 0;
            //uint64_t t_freq_batch_pkt = 0;

            t_ctx.f_pkt_size = 0;

            if( ! f_start_paused )
            {
                LDEBUG( plog, "TF ROACH receiver starting unpaused" );
                out_stream< 0 >().data()->set_pkt_in_session( 0 );
                out_stream< 1 >().data()->set_pkt_in_session( 0 );
                if( ! out_stream< 0 >().set( stream::s_start ) ) return;
                if( ! out_stream< 1 >().set( stream::s_start ) ) return;
                f_time_session_pkt_counter = 0;
                f_freq_session_pkt_counter = 0;
            }

            try
            {
                LPROG( plog, "Starting main loop; waiting for packets" );
                f_break_exe_func.store( true );
                while( f_break_exe_func.load() )
                {
                    f_break_exe_func.store( false );
                    f_exe_func_mutex.lock();
                    if( ! (this->*f_exe_func)( t_ctx ) ) return;
                }
            }
            catch( error& e )
            {
                throw;
            }

            LINFO( plog, "TF ROACH receiver is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output streams" );
            bool t_stop_ok = out_stream< 0 >().set( stream::s_stop ) && out_stream< 1 >().set( stream::s_stop );
            if( ! t_stop_ok ) return;

            LDEBUG( plog, "Exiting output streams" );
            if( ! out_stream< 0 >().set( stream::s_exit ) ) return;
            out_stream< 1 >().set( stream::s_exit );

            return;
        }
        catch( std::exception )
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }

        return;
    }

    bool tf_roach_receiver::exe_time_and_freq( exe_func_context& a_ctx )
    {
        f_exe_func_mutex.unlock();
        bool t_time_pkt_received = !f_force_time_first;

        LDEBUG( plog, "Entering time-and-frequency loop" );
        while( ! is_canceled() && ! f_break_exe_func.load() )
        {
            // check if we've received a pause or unpause instruction
            //try
            //{
            //    check_instruction();
            //}
            //catch( error& e )
            //{
            //    LERROR( plog, e.what() );
            //    break;
            //}

            // the stream::get function is called at the end of the loop so that we can enter the exe func after switching the function pointer
            // and still handle the input command appropriately

            if( a_ctx.f_in_command == stream::s_none )
            {

                LTRACE( plog, "tfrr read s_none" );

            }
            else if( a_ctx.f_in_command == stream::s_error )
            {

                LTRACE( plog, "tfrr read s_error" );
                break;

            }
            else if( a_ctx.f_in_command == stream::s_exit )
            {

                LDEBUG( plog, "TF ROACH receiver is exiting" );
                // Output streams are stopped here because even though this is controlled by the pause/unpause commands,
                // receiving a stop command from upstream overrides that.
                out_stream< 0 >().set( stream::s_exit );
                out_stream< 1 >().set( stream::s_exit );
                return false;

            }
            else if( a_ctx.f_in_command == stream::s_stop )
            {

                LDEBUG( plog, "TF ROACH receiver is stopping" );
                // Output streams are stopped here because even though this is controlled by the pause/unpause commands,
                // receiving a stop command from upstream overrides that.
                if( ! out_stream< 0 >().set( stream::s_stop ) ) break;
                if( ! out_stream< 1 >().set( stream::s_stop ) ) break;

            }
            else if( a_ctx.f_in_command == stream::s_start )
            {

                LDEBUG( plog, "TF ROACH receiver is starting" );
                // Output streams are not started here because this is controlled by the pause/unpause commands

            }
            else
            {
                if( have_instruction() )
                {
                    if( f_paused && use_instruction() == midge::instruction::resume )
                    {
                        LDEBUG( plog, "TF ROACH receiver resuming" );
                        out_stream< 0 >().data()->set_pkt_in_session( 0 );
                        out_stream< 1 >().data()->set_pkt_in_session( 0 );
                        if( ! out_stream< 0 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 0 error while starting";
                        if( ! out_stream< 1 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 1 error while starting";
                        f_time_session_pkt_counter = 0;
                        f_freq_session_pkt_counter = 0;
                        f_paused = false;
                    }
                    else if( ! f_paused && use_instruction() == midge::instruction::pause )
                    {
                        LDEBUG( plog, "TF ROACH receiver pausing" );
                        if( ! out_stream< 0 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 0 error while stopping";
                        if( ! out_stream< 1 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 1 error while stopping";
                        f_paused = true;
                        t_time_pkt_received = !f_force_time_first;
                    }
                }

                // do nothing if paused
                if( ! f_paused && a_ctx.f_in_command == stream::s_run )
                {
                    a_ctx.f_memory_block = in_stream< 0 >().data();

                    a_ctx.f_pkt_size = a_ctx.f_memory_block->get_n_bytes_used();
                    LTRACE( plog, "Handling packet at stream index <" << in_stream< 0 >().get_current_index() << ">; size =  " << a_ctx.f_pkt_size << " bytes; block address = " << (void*)a_ctx.f_memory_block->block() );
                    if( a_ctx.f_pkt_size != f_udp_buffer_size )
                    {
                        LWARN( plog, "Improper packet size; packet may be malformed: received " << a_ctx.f_memory_block->get_n_bytes_used() << " bytes; expected " << f_udp_buffer_size << " bytes" );
                        if( a_ctx.f_pkt_size == 0 ) continue;
                    }

                    byteswap_inplace( reinterpret_cast< raw_roach_packet* >( a_ctx.f_memory_block->block() ) );
                    roach_packet* t_roach_packet = reinterpret_cast< roach_packet* >( a_ctx.f_memory_block->block() );

                    // debug purposes only
    #ifndef NDEBUG
                    raw_roach_packet* t_raw_packet = reinterpret_cast< raw_roach_packet* >( a_ctx.f_memory_block->block() );
                    LTRACE( plog, "Raw packet header: " << std::hex << t_raw_packet->f_word_0 << ", " << t_raw_packet->f_word_1 << ", " << t_raw_packet->f_word_2 << ", " << t_raw_packet->f_word_3 );
    #endif

                    if( t_roach_packet->f_freq_not_time )
                    {
                        // packet is frequency data
                        if( ! t_time_pkt_received ) continue;
                        //t_freq_batch_pkt = t_roach_packet->f_pkt_in_batch;

                        a_ctx.f_freq_data = out_stream< 1 >().data();
                        a_ctx.f_freq_data->set_pkt_in_session( f_freq_session_pkt_counter++ );
                        ::memcpy( &a_ctx.f_freq_data->packet(), t_roach_packet, a_ctx.f_pkt_size );

                        LTRACE( plog, "Frequency data received (" << a_ctx.f_pkt_size << " bytes):  chan = " << a_ctx.f_freq_data->get_digital_id() <<
                               "  time = " << a_ctx.f_freq_data->get_unix_time() <<
                               "  pkt_session = " << a_ctx.f_freq_data->get_pkt_in_session() <<
                               "  pkt_batch = " << t_roach_packet->f_pkt_in_batch <<
                               "  freqNotTime = " << a_ctx.f_freq_data->get_freq_not_time() <<
                               "  first 8 bins: " << (int)a_ctx.f_freq_data->get_array()[ 0 ][ 0 ]  << ", " << (int)a_ctx.f_freq_data->get_array()[ 0 ][ 1 ] << " -- " << (int)a_ctx.f_freq_data->get_array()[ 1 ][ 0 ] << ", " << (int)a_ctx.f_freq_data->get_array()[ 1 ][ 1 ] << " -- " << (int)a_ctx.f_freq_data->get_array()[ 2 ][ 0 ] << ", " << (int)a_ctx.f_freq_data->get_array()[ 2 ][ 1 ] << " -- " << (int)a_ctx.f_freq_data->get_array()[ 3 ][ 0 ] << ", " << (int)a_ctx.f_freq_data->get_array()[ 3 ][ 1 ]);
                        LTRACE( plog, "Frequency data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );
                        if( ! out_stream< 1 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            break;
                        }
                    }
                    else
                    {
                        // packet is time data
                        t_time_pkt_received = true;
                        //t_time_batch_pkt = t_roach_packet->f_pkt_in_batch;

                        a_ctx.f_time_data = out_stream< 0 >().data();
                        a_ctx.f_time_data->set_pkt_in_session( f_time_session_pkt_counter++ );
                        ::memcpy( &a_ctx.f_time_data->packet(), t_roach_packet, a_ctx.f_pkt_size );

                        LTRACE( plog, "Time data received (" << a_ctx.f_pkt_size << " bytes):  chan = " << a_ctx.f_time_data->get_digital_id() <<
                               "  time = " << a_ctx.f_time_data->get_unix_time() <<
                               "  pkt_session = " << a_ctx.f_time_data->get_pkt_in_session() <<
                               "  pkt_batch = " << t_roach_packet->f_pkt_in_batch <<
                               "  freqNotTime = " << a_ctx.f_time_data->get_freq_not_time() <<
                               "  first 8 bins: " << (int)a_ctx.f_time_data->get_array()[ 0 ][ 0 ]  << ", " << (int)a_ctx.f_time_data->get_array()[ 0 ][ 1 ] << " -- " << (int)a_ctx.f_time_data->get_array()[ 1 ][ 0 ] << ", " << (int)a_ctx.f_time_data->get_array()[ 1 ][ 1 ] << " -- " << (int)a_ctx.f_time_data->get_array()[ 2 ][ 0 ] << ", " << (int)a_ctx.f_time_data->get_array()[ 2 ][ 1 ] << " -- " << (int)a_ctx.f_time_data->get_array()[ 3 ][ 0 ] << ", " << (int)a_ctx.f_time_data->get_array()[ 3 ][ 1 ]);
                        LTRACE( plog, "Time data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );
                        if( ! out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            break;
                        }
                    }

                    //if( t_time_batch_pkt == t_freq_batch_pkt )
                    //{
                    //    LTRACE( plog, "Time and frequency batch IDs match: " << t_time_batch_pkt );
                    //    LTRACE( plog, "Frequency data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );
                    //    out_stream< 1 >().set( stream::s_run );
                    //    LTRACE( plog, "Time data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );
                    //    out_stream< 0 >().set( stream::s_run );
                   // }
                } // if block for input command == run

            } // if-else for input command

            a_ctx.f_in_command = in_stream< 0 >().get();
            LTRACE( plog, "TF ROACH receiver reading stream 0 at index " << in_stream< 0 >().get_current_index() );

        } // main while loop

        return true;
    }

    bool tf_roach_receiver::exe_freq_only( exe_func_context& a_ctx )
    {
        f_exe_func_mutex.unlock();

        LDEBUG( plog, "Entering frequency-only loop" );
        while( ! is_canceled() && !  f_break_exe_func.load() )
        {
            // check if we've received a pause or unpause instruction
            //try
            //{
            //    check_instruction();
            //}
            //catch( error& e )
            //{
            //    LERROR( plog, e.what() );
            //    break;
            //}

            // the stream::get function is called at the end of the loop so that we can enter the exe func after switching the function pointer
            // and still handle the input command appropriately

            if( a_ctx.f_in_command == stream::s_none )
            {

                LTRACE( plog, "tfrr read s_none" );

            }
            else if( a_ctx.f_in_command == stream::s_error )
            {

                LTRACE( plog, "tfrr read s_error" );
                break;

            }
            else if( a_ctx.f_in_command == stream::s_exit )
            {

                LDEBUG( plog, "TF ROACH receiver is exiting" );
                // Output streams are stopped here because even though this is controlled by the pause/unpause commands,
                // receiving a stop command from upstream overrides that.
                out_stream< 0 >().set( stream::s_exit );
                out_stream< 1 >().set( stream::s_exit );
                return false;

            }
            else if( a_ctx.f_in_command == stream::s_stop )
            {

                LDEBUG( plog, "TF ROACH receiver is stopping" );
                // Output streams are stopped here because even though this is controlled by the pause/unpause commands,
                // receiving a stop command from upstream overrides that.
                if( ! out_stream< 1 >().set( stream::s_stop ) ) break;

            }
            else if( a_ctx.f_in_command == stream::s_start )
            {
                LDEBUG( plog, "TF ROACH receiver is starting" );
                // Output streams are not started here because this is controled by the pause/unpause commands
                continue;
            }

            else
            {
                // check whether an instruction has been received
                if( have_instruction() )
                {
                    if( f_paused && use_instruction() == midge::instruction::resume )
                    {
                        LDEBUG( plog, "TF ROACH receiver resuming" );
                        out_stream< 1 >().data()->set_pkt_in_session( 0 );
                        if( ! out_stream< 1 >().set( stream::s_start ) ) throw midge::node_nonfatal_error() << "Stream 2 error while starting";
                        f_freq_session_pkt_counter = 0;
                        f_paused = false;
                    }
                    else if( ! f_paused && use_instruction() == midge::instruction::pause )
                    {
                        LDEBUG( plog, "TF ROACH receiver pausing" );
                        if( ! out_stream< 1 >().set( stream::s_stop ) ) throw midge::node_nonfatal_error() << "Stream 2 error while stopping";
                        f_paused = true;
                    }
                }

                // do nothing if paused
                if( ! f_paused && a_ctx.f_in_command == stream::s_run )
                {
                    a_ctx.f_memory_block = in_stream< 0 >().data();

                    a_ctx.f_pkt_size = a_ctx.f_memory_block->get_n_bytes_used();
                    LTRACE( plog, "Handling packet at stream index <" << in_stream< 0 >().get_current_index() << ">; size =  " << a_ctx.f_pkt_size << " bytes; block address = " << (void*)a_ctx.f_memory_block->block() );
                    if( a_ctx.f_pkt_size != f_udp_buffer_size )
                    {
                        LWARN( plog, "Improper packet size; packet may be malformed: received " << a_ctx.f_memory_block->get_n_bytes_used() << " bytes; expected " << f_udp_buffer_size << " bytes" );
                        if( a_ctx.f_pkt_size == 0 ) continue;
                    }

                    byteswap_inplace( reinterpret_cast< raw_roach_packet* >( a_ctx.f_memory_block->block() ) );
                    roach_packet* t_roach_packet = reinterpret_cast< roach_packet* >( a_ctx.f_memory_block->block() );

                    // debug purposes only
    #ifndef NDEBUG
                    raw_roach_packet* t_raw_packet = reinterpret_cast< raw_roach_packet* >( a_ctx.f_memory_block->block() );
                    LTRACE( plog, "Raw packet header: " << std::hex << t_raw_packet->f_word_0 << ", " << t_raw_packet->f_word_1 << ", " << t_raw_packet->f_word_2 << ", " << t_raw_packet->f_word_3 );
    #endif

                    if( t_roach_packet->f_freq_not_time )
                    {
                        // packet is frequency data
                        //t_freq_batch_pkt = t_roach_packet->f_pkt_in_batch;

                        a_ctx.f_freq_data = out_stream< 1 >().data();
                        a_ctx.f_freq_data->set_pkt_in_session( f_freq_session_pkt_counter++ );
                        ::memcpy( &a_ctx.f_freq_data->packet(), t_roach_packet, a_ctx.f_pkt_size );

                        LTRACE( plog, "Frequency data received (" << a_ctx.f_pkt_size << " bytes):  chan = " << a_ctx.f_freq_data->get_digital_id() <<
                               "  time = " << a_ctx.f_freq_data->get_unix_time() <<
                               "  pkt_session = " << a_ctx.f_freq_data->get_pkt_in_session() <<
                               "  pkt_batch = " << t_roach_packet->f_pkt_in_batch <<
                               "  freqNotTime = " << a_ctx.f_freq_data->get_freq_not_time() <<
                               "  first 8 bins: " << (int)a_ctx.f_freq_data->get_array()[ 0 ][ 0 ]  << ", " << (int)a_ctx.f_freq_data->get_array()[ 0 ][ 1 ] << " -- " << (int)a_ctx.f_freq_data->get_array()[ 1 ][ 0 ] << ", " << (int)a_ctx.f_freq_data->get_array()[ 1 ][ 1 ] << " -- " << (int)a_ctx.f_freq_data->get_array()[ 2 ][ 0 ] << ", " << (int)a_ctx.f_freq_data->get_array()[ 2 ][ 1 ] << " -- " << (int)a_ctx.f_freq_data->get_array()[ 3 ][ 0 ] << ", " << (int)a_ctx.f_freq_data->get_array()[ 3 ][ 1 ]);
                        LTRACE( plog, "Frequency data written to stream index <" << out_stream< 1 >().get_current_index() << ">" );
                        if( ! out_stream< 1 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            break;
                        }
                    }
                } // if block for input command == run

            } // if-else block for input command

            a_ctx.f_in_command = in_stream< 0 >().get();
            LTRACE( plog, "TF ROACH receiver reading stream 0 at index " << in_stream< 0 >().get_current_index() );

        } // main while loop

        return true;
    }

    void tf_roach_receiver::finalize()
    {
        out_buffer< 0 >().finalize();
        out_buffer< 1 >().finalize();
        return;
    }

    void tf_roach_receiver::do_cancellation()
    {
        return;
    }


    tf_roach_receiver_binding::tf_roach_receiver_binding() :
            _node_binding< tf_roach_receiver, tf_roach_receiver_binding >()
    {
    }

    tf_roach_receiver_binding::~tf_roach_receiver_binding()
    {
    }

    void tf_roach_receiver_binding::do_apply_config( tf_roach_receiver* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring tf_roach_receiver with:\n" << a_config );
        a_node->set_time_length( a_config.get_value( "time-length", a_node->get_time_length() ) );
        a_node->set_freq_length( a_config.get_value( "freq-length", a_node->get_freq_length() ) );
        a_node->set_udp_buffer_size( a_config.get_value( "udp-buffer-size", a_node->get_udp_buffer_size() ) );
        a_node->set_time_sync_tol( a_config.get_value( "time-sync-tol", a_node->get_time_sync_tol() ) );
        a_node->set_start_paused( a_config.get_value( "start-paused", a_node->get_start_paused() ) );
        a_node->set_force_time_first( a_config.get_value( "force-time-first", a_node->get_force_time_first() ) );
        return;
    }

    void tf_roach_receiver_binding::do_dump_config( const tf_roach_receiver* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping tf_roach_receiver configuration" );
        a_config.add( "time-length", scarab::param_value( a_node->get_time_length() ) );
        a_config.add( "freq-length", scarab::param_value( a_node->get_freq_length() ) );
        a_config.add( "udp-buffer-size", scarab::param_value( a_node->get_udp_buffer_size() ) );
        a_config.add( "time-sync-tol", scarab::param_value( a_node->get_time_sync_tol() ) );
        a_config.add( "start-paused", scarab::param_value( a_node->get_start_paused() ) );
        a_config.add( "force-time-first", scarab::param_value( a_node->get_force_time_first() ) );
        return;

    }

    bool tf_roach_receiver_binding::do_run_command( tf_roach_receiver* a_node, const std::string& a_cmd, const scarab::param_node& ) const
    {
        if( a_cmd == "freq-only" )
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
        }
    }

} /* namespace psyllid */
