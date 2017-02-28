/*
 * frequency_mask_trigger.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: nsoblath
 */

#include "frequency_mask_trigger.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param_codec.hh"
#include "time.hh"

#include <cmath>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( frequency_mask_trigger, "frequency-mask-trigger", frequency_mask_trigger_binding );

    LOGGER( plog, "frequency_mask_trigger" );

    frequency_mask_trigger::frequency_mask_trigger() :
            f_length( 10 ),
            f_n_packets_for_mask( 10 ),
            f_threshold_snr( 3. ),
            f_exe_func( &frequency_mask_trigger::exe_apply_threshold ),
            f_mask(),
            f_n_summed( 0 ),
            f_mask_mutex(),
            f_status( status::mask_update )
    {
    }

    frequency_mask_trigger::~frequency_mask_trigger()
    {
    }

    void frequency_mask_trigger::set_n_packets_for_mask( unsigned a_n_pkts )
    {
        if( a_n_pkts == 0 )
        {
            throw error() << "Number of packets for the trigger mask must be non-zero";
        }
        f_n_packets_for_mask = a_n_pkts;
        return;
    }

    void frequency_mask_trigger::set_threshold_ampl_snr( double a_ampl_snr )
    {
        f_threshold_snr = a_ampl_snr * a_ampl_snr;
        LDEBUG( plog, "Setting threshold (power via ampl) to " << f_threshold_snr );
        return;
    }

    void frequency_mask_trigger::set_threshold_power_snr( double a_power_snr )
    {
        f_threshold_snr = a_power_snr;
        LDEBUG( plog, "Setting threshold (power via power) to " << f_threshold_snr );
        return;
    }

    void frequency_mask_trigger::set_threshold_dB( double a_dB )
    {
        f_threshold_snr = pow( 10, a_dB / 10. );
        LDEBUG( plog, "Setting threshold (power via dB) to " << f_threshold_snr );
        return;
    }


    void frequency_mask_trigger::switch_to_update_mask()
    {
        LDEBUG( plog, "Requesting switch to update-mask mode" );
        f_exe_func_mutex.lock();
        if( f_exe_func != &frequency_mask_trigger::exe_add_to_mask )
        {
            f_break_exe_func.store( true );
            f_status = status::mask_update;
            f_exe_func = &frequency_mask_trigger::exe_add_to_mask;
        }
        f_exe_func_mutex.unlock();
        return;
    }

    void frequency_mask_trigger::switch_to_apply_trigger()
    {
        LDEBUG( plog, "Requesting switch to apply-trigger mode" );
        f_exe_func_mutex.lock();
        if( f_exe_func != &frequency_mask_trigger::exe_apply_threshold )
        {
            f_break_exe_func.store( true );
            f_status = status::triggering;
            f_exe_func = &frequency_mask_trigger::exe_apply_threshold;
        }
        f_exe_func_mutex.unlock();
        return;
    }

    void frequency_mask_trigger::write_mask( const std::string& a_filename )
    {
        std::unique_lock< std::mutex > t_lock( f_mask_mutex );

        if( f_mask.empty() )
        {
            throw error() << "Mask is empty";
        }

        scarab::param_node t_output_node;
        t_output_node.add( "timestamp", new scarab::param_value( scarab::get_absolute_time_string() ) );
        t_output_node.add( "n-packets", new scarab::param_value( f_n_packets_for_mask ) );

        scarab::param_array* t_mask_array = new scarab::param_array();
        t_mask_array->resize( f_mask.size() );
        for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
        {
            t_mask_array->assign( i_bin, new scarab::param_value( f_mask[ i_bin ] ) );
        }
        t_output_node.add( "mask", t_mask_array );

        scarab::param_output_codec* t_json_codec = scarab::factory< scarab::param_output_codec >::get_instance()->create( "json" );
        if( t_json_codec == nullptr )
        {
            throw error() << "Unable to create output-codec for JSON";
        }

        if( ! t_json_codec->write_file( t_output_node, a_filename ) )
        {
            throw error() << "Unable to write mask to file <" << a_filename << ">";
        }

        delete t_json_codec;

        return;
    }

    void frequency_mask_trigger::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void frequency_mask_trigger::execute( midge::diptera* a_midge )
    {
        try
        {
            LINFO( plog, "Starting main loop" );
            f_break_exe_func.store( true );
            while( f_break_exe_func.load() )
            {
                f_break_exe_func.store( false );
                f_exe_func_mutex.lock();
                (this->*f_exe_func)( a_midge );
            }
        }
        catch( error& e )
        {
            throw;
        }
        return;
    }

    void frequency_mask_trigger::exe_add_to_mask( midge::diptera* a_midge )
    {
        f_exe_func_mutex.unlock();

        try
        {
            midge::enum_t t_in_command = stream::s_none;
            freq_data* t_freq_data = nullptr;
            //trigger_flag* t_trigger_flag = nullptr;
            bool t_clear_mask_next_packet = false;
            bool t_mask_is_locked = false; // we can't use try_lock to determine if the mask is locked by this thread (that behavior is undefined), so we use a bool to track it instead
            double t_real = 0., t_imag = 0.;
            unsigned t_array_size = 0;

            LDEBUG( plog, "Entering add-to-mask loop" );
            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                t_in_command = in_stream< 1 >().get();
                if( t_in_command == stream::s_none ) continue;
                if( t_in_command == stream::s_error ) break;

                LTRACE( plog, "FMT (update-mask) reading stream at index " << in_stream< 1 >().get_current_index() );

                t_freq_data = in_stream< 1 >().data();
                //t_trigger_flag = out_stream< 0 >().data();

                if( t_in_command == stream::s_start )
                {
                    f_mask_mutex.lock();
                    t_mask_is_locked = true;
                    LDEBUG( plog, "Starting mask update" ) //; output stream index " << out_stream< 0 >().get_current_index() );
                    //if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    t_clear_mask_next_packet = true;
                    f_n_summed = 0;
                    f_mask_mutex.unlock();
                    t_mask_is_locked = false;
                    continue;
                }

                if( t_in_command == stream::s_run )
                {
                    try
                    {
                        if( f_n_summed >= f_n_packets_for_mask )
                        {
                            LTRACE( plog, "Already have enough packets for the mask; skipping this packet" );
                        }
                        else
                        {
                            f_mask_mutex.lock();
                            t_mask_is_locked = true;
                            LTRACE( plog, "Considering frequency data:  chan = " << t_freq_data->get_digital_id() <<
                                   "  time = " << t_freq_data->get_unix_time() <<
                                   "  id = " << t_freq_data->get_pkt_in_session() <<
                                   "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                                   "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );

                            if( t_clear_mask_next_packet )
                            {
                                t_array_size = t_freq_data->get_array_size();
                                f_mask.resize( t_array_size );
                                for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                                {
                                    f_mask[ i_bin ] = 0.;
                                }
                                t_clear_mask_next_packet = false;
                            }
                            for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                            {
                                t_real = t_freq_data->get_array()[ i_bin ][ 0 ];
                                t_imag = t_freq_data->get_array()[ i_bin ][ 1 ];
                                f_mask[ i_bin ] = f_mask[ i_bin ] + t_real*t_real + t_imag*t_imag;
                    /*#ifndef NDEBUG
                                if( i_bin < 5 )
                                {
                                    LWARN( plog, "Bin " << i_bin << " -- real = " << t_real << ";  imag = " << t_imag << ";  value = " << t_real*t_real + t_imag*t_imag <<
                                            ";  mask = " << f_mask[ i_bin ] );
                                }
                    #endif*/
                            }

                            ++f_n_summed;
                            LTRACE( plog, "Added data to frequency mask; mask now has " << f_n_summed << " packets" );

                            if( f_n_summed == f_n_packets_for_mask )
                            {
                                calculate_mask();
                            }
                            f_mask_mutex.unlock();
                            t_mask_is_locked = false;
                        }

                        // advance the output stream with the trigger set to false
                        // since something else is presumably looking at the time stream, and it needs to stay synchronized
                        //t_trigger_flag->set_id( t_freq_data->get_pkt_in_session() );
                        //t_trigger_flag->set_flag( false );

                        //LTRACE( plog, "FMT writing data to output stream at index " << out_stream< 0 >().get_current_index() );
                        //if( ! out_stream< 0 >().set( stream::s_run ) )
                        //{
                        //    LERROR( plog, "Exiting due to stream error" );
                        //    throw error() << "Stream error while adding to mask";
                        //}
                    }
                    catch( error& e )
                    {
                        LERROR( plog, "Exiting due to error while processing frequency data: " << e.what() );
                        break;
                    }
                    continue;
                }

                if( t_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "FMT is stopping" );// at stream index " << out_stream< 0 >().get_current_index() );
                    //if( ! out_stream< 0 >().set( stream::s_stop ) ) break;
                    continue;
                }

                if( t_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "FMT is exiting" );// at stream index " << out_stream< 0 >().get_current_index() );
                    //out_stream< 0 >().set( stream::s_exit );
                    break;
                }
            }
            // make sure the mutex is unlocked
            if( t_mask_is_locked ) f_mask_mutex.unlock();
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void frequency_mask_trigger::exe_apply_threshold( midge::diptera* a_midge )
    {
        f_exe_func_mutex.unlock();

        try
        {
            midge::enum_t t_in_command = stream::s_none;
            freq_data* t_freq_data = nullptr;
            trigger_flag* t_trigger_flag = nullptr;
            bool t_check_mask_next_packet = false;
            bool t_mask_is_locked = false; // we can't use try_lock to determine if the mask is locked by this thread (that behavior is undefined), so we use a bool to track it instead
            double t_real = 0., t_imag = 0.;
            unsigned t_array_size = 0;

            LDEBUG( plog, "Entering apply-threshold loop" );
            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                t_in_command = in_stream< 0 >().get();
                if( t_in_command == stream::s_none ) continue;
                if( t_in_command == stream::s_error ) break;

                LTRACE( plog, "FMT (apply-threshold) reading stream at index " << in_stream< 0 >().get_current_index() );

                t_freq_data = in_stream< 0 >().data();
                t_trigger_flag = out_stream< 0 >().data();

                if( t_in_command == stream::s_start )
                {
                    f_mask_mutex.lock();
                    t_mask_is_locked = true;
                    LDEBUG( plog, "Starting the FMT; output at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    t_check_mask_next_packet = true;
                    continue;
                }

                if( t_in_command == stream::s_run )
                {
                    LTRACE( plog, "Considering frequency data:  chan = " << t_freq_data->get_digital_id() <<
                           "  time = " << t_freq_data->get_unix_time() <<
                           "  id = " << t_freq_data->get_pkt_in_session() <<
                           "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                           "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );
                    try
                    {
                        if( t_check_mask_next_packet )
                        {
                            if( f_mask.size() != t_array_size ) f_mask.resize( t_array_size, 0. );
                            t_check_mask_next_packet = false;
                        }
                        t_trigger_flag->set_flag( false );

                        t_array_size = t_freq_data->get_array_size();
                        for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                        {
                            t_real = t_freq_data->get_array()[ i_bin ][ 0 ];
                            t_imag = t_freq_data->get_array()[ i_bin ][ 1 ];
                /*#ifndef NDEBUG
                            if( i_bin < 5 )
                            {
                                LWARN( plog, "Bin " << i_bin << " -- real = " << t_real << ";  imag = " << t_imag << ";  value = " << t_real*t_real + t_imag*t_imag <<
                                        ";  mask = " << f_mask[ i_bin ] );
                            }
                #endif*/
                            t_trigger_flag->set_id( t_freq_data->get_pkt_in_session() );
                            if( t_real*t_real + t_imag*t_imag >= f_mask[ i_bin ] )
                            {
                                t_trigger_flag->set_flag( true );
                                LTRACE( plog, "Data " << t_trigger_flag->get_id() << " [bin " << i_bin << "] resulted in flag <" << t_trigger_flag->get_flag() << ">" << '\n' <<
                                       "\tdata: " << t_real*t_real + t_imag*t_imag << ";  mask: " << f_mask[ i_bin ] );
                                break;
                            }
                        }

                        LTRACE( plog, "FMT writing data to output stream at index " << out_stream< 0 >().get_current_index() );
                        if( ! out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            throw error() << "Stream error while applying threshold";
                        }                    }
                    catch( error& e )
                    {
                        LERROR( plog, "Exiting due to error while processing frequency data: " << e.what() );
                        break;
                    }
                    continue;
                }

                if( t_in_command == stream::s_stop )
                {
                    f_mask_mutex.unlock();
                    t_mask_is_locked = false;
                    LDEBUG( plog, "FMT is stopping at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_stop ) ) break;
                    continue;
                }

                if( t_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "FMT is exiting at stream index " << out_stream< 0 >().get_current_index() );
                    out_stream< 0 >().set( stream::s_exit );
                    break;
                }
            }
            // make sure the mutex is unlocked
            if( t_mask_is_locked ) f_mask_mutex.unlock();
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void frequency_mask_trigger::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }

    void frequency_mask_trigger::calculate_mask()
    {
        LDEBUG( plog, "Calculating frequency mask" );

        f_status = status::triggering;
        f_exe_func = &frequency_mask_trigger::exe_apply_threshold;

        double t_multiplier = f_threshold_snr / (double)f_n_summed;
        for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
        {
/*#ifndef NDEBUG
            if( i_bin < 5 )
            {
                LWARN( plog, "Bin " << i_bin << " -- threshold snr = " << f_threshold_snr << ";  n_summed = " << f_n_summed << ";  multiplier = " << t_multiplier <<
                        ";  summed value = " << f_mask[ i_bin ] << ";  final mask = " << f_mask[ i_bin ] * t_multiplier );
            }
#endif*/
            f_mask[ i_bin ] = f_mask[ i_bin ] * t_multiplier;
        }

        return;
    }


    frequency_mask_trigger_binding::frequency_mask_trigger_binding() :
            _node_binding< frequency_mask_trigger, frequency_mask_trigger_binding >()
    {
    }

    frequency_mask_trigger_binding::~frequency_mask_trigger_binding()
    {
    }

    void frequency_mask_trigger_binding::do_apply_config( frequency_mask_trigger* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring frequency_mask_trigger with:\n" << a_config );
        a_node->set_n_packets_for_mask( a_config.get_value( "n-packets-for-mask", a_node->get_n_packets_for_mask() ) );

        if( a_config.has( "threshold-ampl-snr" ) )
        {
            a_node->set_threshold_ampl_snr( a_config.get_value< double >( "threshold-ampl-snr" ) );
        }
        if( a_config.has( "threshold-power-snr" ) )
        {
            a_node->set_threshold_power_snr( a_config.get_value< double >( "threshold-power-snr" ) );
        }
        if( a_config.has( "threshold-db" ) )
        {
            a_node->set_threshold_dB( a_config.get_value< double >( "threshold-db" ) );
        }

        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        return;
    }

    void frequency_mask_trigger_binding::do_dump_config( const frequency_mask_trigger* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for frequency_mask_trigger" );
        a_config.add( "n-packets-for-mask", new scarab::param_value( a_node->get_n_packets_for_mask() ) );
        a_config.add( "threshold-power-snr", new scarab::param_value( a_node->get_threshold_snr() ) );
        a_config.add( "length", new scarab::param_value( a_node->get_length() ) );
        return;
    }

    bool frequency_mask_trigger_binding::do_run_command( frequency_mask_trigger* a_node, const std::string& a_cmd, const scarab::param_node& a_args ) const
    {
        if( a_cmd == "update-mask" )
        {
            a_node->switch_to_update_mask();
            return true;
        }
        else if( a_cmd == "apply-trigger" )
        {
            a_node->switch_to_apply_trigger();
            return true;
        }
        else if( a_cmd == "write-mask" )
        {
            try
            {
                a_node->write_mask( a_args.get_value( "filename", "fmt_mask.json" ) );
            }
            catch( error& e )
            {
                throw e;
            }
            return true;
        }
        else
        {
            LWARN( plog, "Unrecognized command: <" << a_cmd << ">" );
            return false;
        }
    }


} /* namespace psyllid */
