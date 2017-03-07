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

#include "tk_spline.hh"

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
            f_n_spline_points( 20 ),
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
        exe_func_context t_ctx;
        t_ctx.f_midge = a_midge;
        t_ctx.f_first_packet_after_start = false;

        try
        {
            LINFO( plog, "Starting main loop" );
            f_break_exe_func.store( true );
            while( f_break_exe_func.load() )
            {
                f_break_exe_func.store( false );
                f_exe_func_mutex.lock();
                (this->*f_exe_func)( t_ctx );
            }
        }
        catch( error& e )
        {
            throw;
        }
        return;
    }

    void frequency_mask_trigger::exe_add_to_mask( exe_func_context& a_ctx )
    {
        f_exe_func_mutex.unlock();

        try
        {
            midge::enum_t t_in_command = stream::s_none;
            freq_data* t_freq_data = nullptr;
            //trigger_flag* t_trigger_flag = nullptr;
            double t_real = 0., t_imag = 0.;
            unsigned t_array_size = 0;
            std::vector< double > t_mask_buffer;

            LDEBUG( plog, "Entering add-to-mask loop" );
            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                t_in_command = in_stream< 0 >().get();
                if( t_in_command == stream::s_none ) continue;
                if( t_in_command == stream::s_error ) break;

                LTRACE( plog, "FMT (update-mask) reading stream at index " << in_stream< 0 >().get_current_index() );

                t_freq_data = in_stream< 0 >().data();
                //t_trigger_flag = out_stream< 0 >().data();

                if( t_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting mask update" ) //; output stream index " << out_stream< 0 >().get_current_index() );
                    //if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    a_ctx.f_first_packet_after_start = true;
                    f_n_summed = 0;
                    t_mask_buffer.clear();
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

                            LTRACE( plog, "Considering frequency data:  chan = " << t_freq_data->get_digital_id() <<
                                   "  time = " << t_freq_data->get_unix_time() <<
                                   "  id = " << t_freq_data->get_pkt_in_session() <<
                                   "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                                   "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );

                            if( a_ctx.f_first_packet_after_start )
                            {
                                t_array_size = t_freq_data->get_array_size();
                                LWARN( plog, "changing the size of the array buffer to " << t_array_size );
                                t_mask_buffer.resize( t_array_size );
                                for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                                {
                                    t_mask_buffer[ i_bin ] = 0.;
                                }
                                a_ctx.f_first_packet_after_start = false;
                            }
                            for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                            {
                                t_real = t_freq_data->get_array()[ i_bin ][ 0 ];
                                t_imag = t_freq_data->get_array()[ i_bin ][ 1 ];
                                t_mask_buffer[ i_bin ] = t_mask_buffer[ i_bin ] + t_real*t_real + t_imag*t_imag;
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
                                LDEBUG( plog, "Calculating spline for frequency mask" );

                                double t_multiplier = f_threshold_snr / (double)f_n_summed;
                                LDEBUG( plog, "Size: " << t_mask_buffer.size() << "   Multiplier = " << t_multiplier << " = (threshold_snr) " << f_threshold_snr << " / (n_summed) " << f_n_summed );

                                std::vector< double > t_x_vals( f_n_spline_points );
                                std::vector< double > t_y_vals( f_n_spline_points );
                                unsigned t_n_bins_per_point = t_mask_buffer.size() / f_n_spline_points;

                                // calculate spline points
                                for( unsigned i_spline_point = 0; i_spline_point < f_n_spline_points; ++i_spline_point )
                                {
                                    unsigned t_bin_begin = i_spline_point * t_n_bins_per_point;
                                    unsigned t_bin_end = i_spline_point == f_n_spline_points - 1 ? t_mask_buffer.size() : t_bin_begin + t_n_bins_per_point;
                                    double t_mean = 0.;
                                    for( unsigned i_bin = t_bin_begin; i_bin < t_bin_end; ++i_bin )
                                    {
                                        t_mean += t_mask_buffer[ i_bin ];
                                    }
                                    t_mean *= t_multiplier / (double)(t_bin_end - t_bin_begin);
                                    t_y_vals[ i_spline_point ] = t_mean;
                                    t_x_vals[ i_spline_point ] = (double)t_bin_begin + 0.5 * (double)(t_bin_end - 1 - t_bin_begin);
                                }

                                // create the spline
                                tk::spline t_spline;
                                t_spline.set_points( t_x_vals, t_y_vals );

                                f_mask_mutex.lock();
                                LDEBUG( plog, "Calculating frequency mask" );

                                f_mask.resize( t_mask_buffer.size() );
                                for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
                                {
                                    f_mask[ i_bin ] = t_spline( i_bin );
                                }

                                f_mask_mutex.unlock();
                            }
                        }
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
                    if( f_n_summed < f_n_packets_for_mask )
                    {
                        LWARN( plog, "FMT is stopping: it did not process enough packets to update the mask" );
                    }
                    //if( ! out_stream< 0 >().set( stream::s_stop ) ) break;
                    continue;
                }

                if( t_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "FMT is exiting" );// at stream index " << out_stream< 0 >().get_current_index() );
                    if( f_n_summed < f_n_packets_for_mask )
                    {
                        LWARN( plog, "FMT is exiting: it did not process enough packets to update the mask" );
                    }
                    //out_stream< 0 >().set( stream::s_exit );
                    break;
                }
            }
        }
        catch(...)
        {
            if( a_ctx.f_midge ) a_ctx.f_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void frequency_mask_trigger::exe_apply_threshold( exe_func_context& a_ctx )
    {
        f_exe_func_mutex.unlock();

        try
        {
            midge::enum_t t_in_command = stream::s_none;
            freq_data* t_freq_data = nullptr;
            trigger_flag* t_trigger_flag = nullptr;
            double t_real = 0., t_imag = 0.;
            unsigned t_array_size = 0;

            f_mask_mutex.lock();
            std::vector< double > t_mask_buffer( f_mask );
            f_mask_mutex.unlock();

            LDEBUG( plog, "Entering apply-threshold loop" );
            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                t_in_command = in_stream< 0 >().get();
                if( t_in_command == stream::s_none )
                {
                    LTRACE( plog, "FMT read s_none" );
                    continue;
                }
                if( t_in_command == stream::s_error )
                {
                    LTRACE( plog, "FMT read s_error" );
                    break;
                }

                LTRACE( plog, "FMT (apply-threshold) reading stream at index " << in_stream< 0 >().get_current_index() );

                t_freq_data = in_stream< 0 >().data();
                t_trigger_flag = out_stream< 0 >().data();

                if( t_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting the FMT; output at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    a_ctx.f_first_packet_after_start = true;
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
                        t_array_size = t_freq_data->get_array_size();
                        if( a_ctx.f_first_packet_after_start )
                        {
                            LDEBUG( plog, "Resizing mask to " << t_array_size << " bins" );
                            if( t_mask_buffer.size() != t_array_size ) t_mask_buffer.resize( t_array_size, 0. );
                            a_ctx.f_first_packet_after_start = false;
                        }
                        t_trigger_flag->set_flag( false );

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
                            if( t_real*t_real + t_imag*t_imag >= t_mask_buffer[ i_bin ] )
                            {
                                t_trigger_flag->set_flag( true );
                                LTRACE( plog, "Data " << t_trigger_flag->get_id() << " [bin " << i_bin << "] resulted in flag <" << t_trigger_flag->get_flag() << ">" << '\n' <<
                                       "\tdata: " << t_real*t_real + t_imag*t_imag << ";  mask: " << t_mask_buffer[ i_bin ] );
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
        }
        catch(...)
        {
            if( a_ctx.f_midge ) a_ctx.f_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void frequency_mask_trigger::finalize()
    {
        out_buffer< 0 >().finalize();
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
        a_node->set_n_spline_points( a_config.get_value( "n-spline-points", a_node->get_n_spline_points() ) );

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
        a_config.add( "n-spline-points", new scarab::param_value( a_node->get_n_spline_points() ) );
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
