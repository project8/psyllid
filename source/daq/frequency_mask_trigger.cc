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

    // trigger_mode_t utility functions
    std::string frequency_mask_trigger::trigger_mode_to_string( frequency_mask_trigger::trigger_mode_t a_trigger_mode )
    {
        // note that the string representations use hyphens, not underscores
        switch (a_trigger_mode) {
            case frequency_mask_trigger::trigger_mode_t::single_level: return "single-level";
            case frequency_mask_trigger::trigger_mode_t::two_level: return "two-level";
            default: throw psyllid::error() << "trigger_mode value <" << trigger_mode_to_uint(a_trigger_mode) << "> not recognized";
        }
    }
    frequency_mask_trigger::trigger_mode_t frequency_mask_trigger::string_to_trigger_mode( const std::string& a_trigger_mode_string )
    {
        if ( a_trigger_mode_string == trigger_mode_to_string( frequency_mask_trigger::trigger_mode_t::single_level ) ) return trigger_mode_t::single_level;
        if ( a_trigger_mode_string == trigger_mode_to_string( frequency_mask_trigger::trigger_mode_t::two_level ) ) return trigger_mode_t::two_level;
        throw psyllid::error() << "string <" << a_trigger_mode_string << "> not recognized as valid trigger_mode type";
    }

    // threshold_t utility functions
    std::string frequency_mask_trigger::threshold_to_string( frequency_mask_trigger::threshold_t a_threshold )
    {
        switch (a_threshold) {
            case frequency_mask_trigger::threshold_t::snr: return "snr";
            case frequency_mask_trigger::threshold_t::sigma: return "sigma";
            default: throw psyllid::error() << "threshold value <" << threshold_to_uint(a_threshold) << "> not recognized";
        }
    }
    frequency_mask_trigger::threshold_t frequency_mask_trigger::string_to_threshold( const std::string& a_threshold_string )
    {
        if ( a_threshold_string == threshold_to_string( frequency_mask_trigger::threshold_t::snr ) ) return threshold_t::snr;
        if ( a_threshold_string == threshold_to_string( frequency_mask_trigger::threshold_t::sigma ) ) return threshold_t::sigma;
        throw psyllid::error() << "string <" << a_threshold_string << "> not recognized as valid threshold type";
    }

    frequency_mask_trigger::frequency_mask_trigger() :
            f_length( 10 ),
            f_n_packets_for_mask( 10 ),
            f_threshold_snr( 30. ),
            f_threshold_snr_high( 30. ),
            f_threshold_sigma( 30 ),
            f_threshold_sigma_high( 30 ),
            f_threshold_type( threshold_t::sigma ),
            f_n_spline_points( 20 ),
            f_status( status_t::mask_update ),
            f_trigger_mode(trigger_mode_t::single_level ),
            f_n_excluded_bins( 0 ),
            f_exe_func( &frequency_mask_trigger::exe_apply_threshold ),
            f_mask(),
            f_mask2(),
            f_average_data(),
            f_variance_data(),
            f_n_summed( 0 ),
            f_mask_mutex()
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

    void frequency_mask_trigger::set_threshold_dB( double a_dB )
    {
        f_threshold_snr = pow( 10, a_dB / 10. );
        LDEBUG( plog, "Setting threshold (power via dB) to " << f_threshold_snr );
        return;
    }

    void frequency_mask_trigger::calculate_sigma_mask_spline_points( std::vector< double >& t_x_vals, std::vector< double >& t_y_vals, double threshold )
    {
        unsigned t_n_bins_per_point = f_average_data.size() / f_n_spline_points;
        for( unsigned i_spline_point = 0; i_spline_point < f_n_spline_points; ++i_spline_point )
        {
            unsigned t_bin_begin = i_spline_point * t_n_bins_per_point;
            unsigned t_bin_end = i_spline_point == f_n_spline_points - 1 ? f_average_data.size() : t_bin_begin + t_n_bins_per_point;
            double t_mean = 0.;
            for( unsigned i_bin = t_bin_begin; i_bin < t_bin_end; ++i_bin )
            {
                t_mean += f_average_data[ i_bin ] + threshold * sqrt( f_variance_data[ i_bin ] );
            }
            t_mean *= 1. / (double)(t_bin_end - t_bin_begin);
            t_y_vals[ i_spline_point ] = t_mean;
            t_x_vals[ i_spline_point ] = (double)t_bin_begin + 0.5 * (double)(t_bin_end - 1 - t_bin_begin);
        }
    }

    void frequency_mask_trigger::calculate_snr_mask_spline_points( std::vector< double >& t_x_vals, std::vector< double >& t_y_vals, double threshold )
    {
        unsigned t_n_bins_per_point = f_average_data.size() / f_n_spline_points;
        for( unsigned i_spline_point = 0; i_spline_point < f_n_spline_points; ++i_spline_point )
        {
            unsigned t_bin_begin = i_spline_point * t_n_bins_per_point;
            unsigned t_bin_end = i_spline_point == f_n_spline_points - 1 ? f_average_data.size() : t_bin_begin + t_n_bins_per_point;
            double t_mean = 0.;
            for( unsigned i_bin = t_bin_begin; i_bin < t_bin_end; ++i_bin )
            {
                t_mean += f_average_data[ i_bin ] * threshold;
            }
            t_mean *= 1. / (double)(t_bin_end - t_bin_begin);
            t_y_vals[ i_spline_point ] = t_mean;
            t_x_vals[ i_spline_point ] = (double)t_bin_begin + 0.5 * (double)(t_bin_end - 1 - t_bin_begin);
        }
    }

    void frequency_mask_trigger::set_mask_parameters_from_node( const scarab::param_node& a_mask_and_data_values )
    {
        // set n-points
        f_n_packets_for_mask = a_mask_and_data_values["n-packets"]().as_uint();
        // grab the new arrays
        const scarab::param_array t_new_mask = a_mask_and_data_values["mask"].as_array();
        const scarab::param_array t_new_mask2 = a_mask_and_data_values["mask2"].as_array();
        const scarab::param_array t_new_data_mean = a_mask_and_data_values["data-mean"].as_array();
        const scarab::param_array t_new_data_variance = a_mask_and_data_values["data-variance"].as_array();
        LDEBUG( plog, "Finished reading mask" );
        // prep the data members
        f_mask.clear();
        f_mask2.clear();
        f_average_data.clear();
        f_variance_data.clear();
        f_mask.resize( t_new_mask.size() );
        f_mask2.resize( 0 );
        f_average_data.resize( t_new_data_mean.size() );
        f_variance_data.resize( t_new_data_variance.size() );

        // assign new values
        for( unsigned i_bin = 0; i_bin < t_new_mask.size(); ++i_bin )
        {
            f_mask[ i_bin ] = t_new_mask[i_bin]().as_double();
            f_average_data[ i_bin ] = t_new_data_mean[i_bin]().as_double();
            f_variance_data[ i_bin ] = t_new_data_variance[i_bin]().as_double();
        }
        //TODO what case are we covering here, and what should we do?
        //if ( t_new_mask2 != nullptr )
        //{
        if ( t_new_mask2.size() != t_new_mask.size() ) throw psyllid::error() << "new mask and new mask2 must have same size";

        f_mask2.resize( t_new_mask2.size() );
        for( unsigned i_bin = 0; i_bin < t_new_mask2.size(); ++i_bin )
        {
            f_mask2[ i_bin ] = t_new_mask2[i_bin]().as_double();
        }
        //}
    }

    void frequency_mask_trigger::switch_to_update_mask()
    {
        LDEBUG( plog, "Requesting switch to update-mask mode" );
        f_exe_func_mutex.lock();
        if( f_exe_func != &frequency_mask_trigger::exe_add_to_mask )
        {
            f_break_exe_func.store( true );
            f_status = status_t::mask_update;
            f_exe_func = &frequency_mask_trigger::exe_add_to_mask;
        }
        f_exe_func_mutex.unlock();
        return;
    }

    void frequency_mask_trigger::switch_to_apply_trigger()
    {
        LDEBUG( plog, "Requesting switch to apply-trigger mode" );
        f_exe_func_mutex.lock();
        if ( f_trigger_mode == trigger_mode_t::single_level)
        {
            if ( f_exe_func != &frequency_mask_trigger::exe_apply_threshold )
            {
                f_break_exe_func.store( true );
                f_status = status_t::triggering;
                f_exe_func = &frequency_mask_trigger::exe_apply_threshold;
            }
        }
        else
        {
            LDEBUG( plog, "Switching to exe_apply_two_thresholds" );
            if ( f_exe_func != &frequency_mask_trigger::exe_apply_two_thresholds )
            {
                LDEBUG( plog, "Break exe_func" );
                f_break_exe_func.store( true );
                f_status = status_t::triggering;
                f_exe_func = &frequency_mask_trigger::exe_apply_two_thresholds;
            }
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
        t_output_node.add( "timestamp", scarab::param_value( scarab::get_formatted_now() ) );
        t_output_node.add( "n-packets", scarab::param_value( f_n_packets_for_mask ) );

        scarab::param_array t_mask_array = scarab::param_array();
        t_mask_array.resize( f_mask.size() );
        for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
        {
            t_mask_array.assign( i_bin, scarab::param_value( f_mask[ i_bin ] ) );
        }
        t_output_node.add( "mask", t_mask_array );

        if ( !f_mask2.empty() )
        {
            scarab::param_array t_mask_array2 = scarab::param_array();
            t_mask_array2.resize( f_mask2.size() );
            for( unsigned i_bin = 0; i_bin < f_mask2.size(); ++i_bin )
            {
                t_mask_array2.assign( i_bin, scarab::param_value( f_mask2[ i_bin ] ) );
            }
            t_output_node.add( "mask2", t_mask_array2 );
        }


        scarab::param_array t_mean_data_array = scarab::param_array();
        scarab::param_array t_variance_data_array = scarab::param_array();
        t_mean_data_array.resize( f_average_data.size() );
        t_variance_data_array.resize( f_variance_data.size() );
        for( unsigned i_bin = 0; i_bin < f_average_data.size(); ++i_bin )
        {
            t_mean_data_array.assign( i_bin, scarab::param_value( f_average_data[ i_bin ] ) );
            t_variance_data_array.assign( i_bin, scarab::param_value( f_variance_data[ i_bin ] ) );
        }
        t_output_node.add( "data-mean", t_mean_data_array );
        t_output_node.add( "data-variance", t_variance_data_array );

        scarab::param_translator t_param_translator = scarab::param_translator();

        LTRACE( plog, "Mask file:\n" << t_output_node );
        if( ! t_param_translator.write_file( t_output_node, a_filename ) )
        {
            throw error() << "Unable to write mask to file <" << a_filename << ">";
        }

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
        t_ctx.f_in_command = stream::s_none;

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
        LINFO( plog, "FMT has exited" );
        return;
    }

    void frequency_mask_trigger::exe_add_to_mask( exe_func_context& a_ctx )
    {
        f_exe_func_mutex.unlock();

        try
        {
            freq_data* t_freq_data = nullptr;
            double t_real = 0., t_imag = 0., t_abs_square = 0.;
            unsigned t_array_size = 0;

            LDEBUG( plog, "Entering add-to-mask loop" );
            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                // the stream::get function is called at the end of the loop so
                // that we can enter the exe func after switching the function pointer
                // and still handle the input command appropriately

                if( a_ctx.f_in_command == stream::s_none )
                {
                    LTRACE( plog, "FMT read s_none" );
                }
                else if( a_ctx.f_in_command == stream::s_error )
                {
                    LTRACE( plog, "FMT read s_error" );
                    break;
                }
                else if( a_ctx.f_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting mask update" );
                    a_ctx.f_first_packet_after_start = true;
                    f_n_summed = 0;
                    f_average_data.clear();
                    f_variance_data.clear();
                }
                else if( a_ctx.f_in_command == stream::s_run )
                {
                    t_freq_data = in_stream< 0 >().data();

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
                                f_average_data.resize( t_array_size );
                                f_variance_data.resize( t_array_size );
                                for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                                {
                                    f_average_data[ i_bin ] = 0.;
                                    f_variance_data[ i_bin ] = 0.;
                                }
                                a_ctx.f_first_packet_after_start = false;
                            }
                            for( unsigned i_bin = 0; i_bin < t_array_size; ++i_bin )
                            {
                                t_real = t_freq_data->get_array()[ i_bin ][ 0 ];
                                t_imag = t_freq_data->get_array()[ i_bin ][ 1 ];
                                t_abs_square = t_real*t_real + t_imag*t_imag;
                                f_variance_data[ i_bin ] = f_variance_data[ i_bin ] + t_abs_square * t_abs_square;
                                f_average_data[ i_bin ] = f_average_data[ i_bin ] +  t_abs_square;
                            }

                            ++f_n_summed;
                            LTRACE( plog, "Added data to frequency mask; mask now has " << f_n_summed << " packets" );

                            if( f_n_summed == f_n_packets_for_mask )
                            {
                                // calculate average and variance
                                for( unsigned i_bin = 0; i_bin < f_average_data.size(); ++i_bin )
                                {
                                    f_variance_data [ i_bin ] = ( f_variance_data [ i_bin ] - f_average_data [ i_bin ] * f_average_data [ i_bin ] / (double) f_n_summed ) /( (double) f_n_summed -1 );
                                    f_average_data[ i_bin ] = f_average_data[ i_bin ]/ (double) f_n_summed;
                                }

                                LDEBUG( plog, "Calculating spline for frequency mask" );
                                std::vector< double > t_x_vals( f_n_spline_points );
                                std::vector< double > t_y_vals( f_n_spline_points );

                                if ( f_threshold_type == threshold_t::sigma )
                                {
                                    calculate_sigma_mask_spline_points(t_x_vals, t_y_vals, f_threshold_sigma);

                                    // create the spline
                                    tk::spline t_spline;
                                    t_spline.set_points( t_x_vals, t_y_vals );

                                    f_mask_mutex.lock();
                                    LDEBUG( plog, "Calculating frequency sigma mask" );
                                    f_mask.resize( f_average_data.size() );
                                    for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
                                    {
                                        f_mask[ i_bin ] = t_spline( i_bin );
                                    }

                                    if ( f_trigger_mode == trigger_mode_t::two_level )
                                    {
                                        calculate_sigma_mask_spline_points(t_x_vals, t_y_vals, f_threshold_sigma_high);
                                        // create the spline
                                        tk::spline t_spline;
                                        t_spline.set_points( t_x_vals, t_y_vals );

                                        LDEBUG( plog, "Calculating frequency sigma mask2" );

                                        f_mask2.resize( f_average_data.size() );
                                        for( unsigned i_bin = 0; i_bin < f_mask2.size(); ++i_bin )
                                        {
                                            f_mask2[ i_bin ] = t_spline( i_bin );
                                        }
                                    }
                                }
                                else
                                {
                                    calculate_snr_mask_spline_points(t_x_vals, t_y_vals, f_threshold_snr);

                                    // create the spline
                                    tk::spline t_spline;
                                    t_spline.set_points( t_x_vals, t_y_vals );

                                    f_mask_mutex.lock();
                                    LDEBUG( plog, "Calculating frequency snr mask" );

                                    f_mask.resize( f_average_data.size() );
                                    for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
                                    {
                                        f_mask[ i_bin ] = t_spline( i_bin );
                                    }

                                    if ( f_trigger_mode == trigger_mode_t::two_level )
                                    {
                                        calculate_snr_mask_spline_points(t_x_vals, t_y_vals, f_threshold_snr_high);
                                        // create the spline
                                        tk::spline t_spline;
                                        t_spline.set_points( t_x_vals, t_y_vals );

                                        LDEBUG( plog, "Calculating frequency snr mask2" );

                                        f_mask2.resize( f_average_data.size() );
                                        for( unsigned i_bin = 0; i_bin < f_mask2.size(); ++i_bin )
                                        {
                                            f_mask2[ i_bin ] = t_spline( i_bin );
                                        }
                                    }
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

                }
                else if( a_ctx.f_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "FMT is stopping" );
                    if( f_n_summed < f_n_packets_for_mask )
                    {
                        LWARN( plog, "FMT is stopping: it did not process enough packets to update the mask" );
                    }
                }
                else if( a_ctx.f_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "FMT is exiting" );
                    if( f_n_summed < f_n_packets_for_mask )
                    {
                        LWARN( plog, "FMT is exiting: it did not process enough packets to update the mask" );
                    }
                    break;

                }

                a_ctx.f_in_command = in_stream< 0 >().get();
                LTRACE( plog, "FMT (update-mask) reading stream at index " << in_stream< 0 >().get_current_index() );

            } // end while( ! is_canceled() && ! a_ctx.f_break_exe_loop() )

            LDEBUG( plog, "FMT has exited the add-to-mask while loop; possible reasons: is_canceled() = " << is_canceled() << "; f_break_exe_func.load() = " << f_break_exe_func.load() );
            if( f_break_exe_func.load() )
            {
                LINFO( plog, "FMT is switching exe while loops" );
                return;
            }
            else
            {
                LINFO( plog, "FMT is exiting" );
            }

            LDEBUG( plog, "Stopping output stream" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

            return;
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
            freq_data* t_freq_data = nullptr;
            trigger_flag* t_trigger_flag = nullptr;
            double t_real = 0., t_imag = 0., t_power_amp = 0.;
            unsigned t_array_size = 0;
            unsigned t_loop_lower_limit = 0;
            unsigned t_loop_upper_limit = 0;

            f_mask_mutex.lock();
            std::vector< double > t_mask_buffer( f_mask );
            f_mask_mutex.unlock();

            LDEBUG( plog, "Entering apply-threshold loop" );
            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                // the stream::get function is called at the end of the loop so
                // that we can enter the exe func after switching the function pointer
                // and still handle the input command appropriately

                if( a_ctx.f_in_command == stream::s_none )
                {
                    LTRACE( plog, "FMT read s_none" );
                }
                else if( a_ctx.f_in_command == stream::s_error )
                {
                    LTRACE( plog, "FMT read s_error" );
                    break;
                }
                else if( a_ctx.f_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting the FMT; output at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    a_ctx.f_first_packet_after_start = true;
                }
                if( a_ctx.f_in_command == stream::s_run )
                {
                    t_freq_data = in_stream< 0 >().data();
                    t_trigger_flag = out_stream< 0 >().data();

                    LTRACE( plog, "Considering frequency data:  chan = " << t_freq_data->get_digital_id() <<
                           "  time = " << t_freq_data->get_unix_time() <<
                           "  id = " << t_freq_data->get_pkt_in_session() <<
                           "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                           "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );
                    try
                    {
                        t_array_size = t_freq_data->get_array_size();
                        t_loop_lower_limit = f_n_excluded_bins;
                        t_loop_upper_limit = t_array_size - f_n_excluded_bins;
                        LDEBUG( plog, "Array size: "<<t_array_size );
                        LDEBUG( plog, "Looping from "<<t_loop_lower_limit<<" to "<<t_loop_upper_limit-1 );

                        if( a_ctx.f_first_packet_after_start )
                        {
                            if( t_mask_buffer.size() != t_array_size )
                            {
                                throw psyllid::error() << "Frequency mask is not the same size as frequency data array";
                            }
                            a_ctx.f_first_packet_after_start = false;
                        }

                        t_trigger_flag->set_flag( false );
                        t_trigger_flag->set_high_threshold( false );
                        t_trigger_flag->set_id( t_freq_data->get_pkt_in_session() );

                        for( unsigned i_bin = t_loop_lower_limit; i_bin < t_loop_upper_limit; ++i_bin )
                        {
                            t_real = t_freq_data->get_array()[ i_bin ][ 0 ];
                            t_imag = t_freq_data->get_array()[ i_bin ][ 1 ];
                            t_power_amp = t_real*t_real + t_imag*t_imag;

                            if( t_power_amp >= t_mask_buffer[ i_bin ] )
                            {
                                t_trigger_flag->set_flag( true );
                                t_trigger_flag->set_high_threshold( true );
                                LDEBUG( plog, "Data id <" << t_trigger_flag->get_id() << "> [bin " << i_bin <<
                                       "] resulted in flag <" << t_trigger_flag->get_flag() << ">" << '\n' <<
                                       "\tdata: " << t_power_amp << ";  mask1: " << t_mask_buffer[ i_bin ] );
                                break;
                            }
                        }
#ifndef NDEBUG
                        if( ! t_trigger_flag->get_flag() )
                        {
                            LTRACE( plog, "Data id <" << t_trigger_flag->get_id() << "> resulted in flag <" << t_trigger_flag->get_flag() << ">");
                        }
#endif

                        LTRACE( plog, "FMT writing data to output stream at index " << out_stream< 0 >().get_current_index() );
                        if( ! out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            throw midge::node_nonfatal_error() << "Stream error while applying threshold";
                        }
                    }
                    catch( error& e )
                    {
                        LERROR( plog, "Exiting due to error while processing frequency data: " << e.what() );
                        throw;
                    }

                }
                else if( a_ctx.f_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "FMT is stopping at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_stop ) ) break;
                }
                else if( a_ctx.f_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "FMT is exiting at stream index " << out_stream< 0 >().get_current_index() );
                    out_stream< 0 >().set( stream::s_exit );
                    break;
                }

                a_ctx.f_in_command = in_stream< 0 >().get();
                LTRACE( plog, "FMT (apply-threshold) reading stream at index " << in_stream< 0 >().get_current_index() );

            } // while( ! is_canceled() && ! f_break_exe_func.load() )

            LDEBUG( plog, "FMT has exited the apply-threshold while loop; possible reasons: is_canceled() = " <<
                   is_canceled() << "; f_break_exe_func.load() = " << f_break_exe_func.load() );
            if( f_break_exe_func.load() )
            {
                LINFO( plog, "FMT is switching exe while loops" );
                return;
            }
            else
            {
                LINFO( plog, "FMT is exiting" );
            }

            LDEBUG( plog, "Stopping output stream" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

            return;
        }
        catch(...)
        {
            if( a_ctx.f_midge ) a_ctx.f_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }


    void frequency_mask_trigger::exe_apply_two_thresholds( exe_func_context& a_ctx )
    {
        f_exe_func_mutex.unlock();

        try
        {
            freq_data* t_freq_data = nullptr;
            trigger_flag* t_trigger_flag = nullptr;
            double t_real = 0., t_imag = 0., t_power_amp = 0.;
            unsigned t_array_size = 0;
            unsigned t_loop_lower_limit = 0;
            unsigned t_loop_upper_limit = 0;


            f_mask_mutex.lock();
            std::vector< double > t_mask_buffer( f_mask );
            std::vector< double > t_mask2_buffer( f_mask2 );

            LDEBUG( plog, "mask sizes: " << t_mask_buffer.size() << " " << t_mask2_buffer.size() );

            f_mask_mutex.unlock();

            LDEBUG( plog, "Entering apply-two-thresholds loop" );

            while( ! is_canceled() && ! f_break_exe_func.load() )
            {
                // the stream::get function is called at the end of the loop so
                // that we can enter the exe func after switching the function pointer
                // and still handle the input command appropriately

                if( a_ctx.f_in_command == stream::s_none )
                {
                    LTRACE( plog, "FMT read s_none" );
                }
                else if( a_ctx.f_in_command == stream::s_error )
                {
                    LTRACE( plog, "FMT read s_error" );
                    break;
                }
                else if( a_ctx.f_in_command == stream::s_start )
                {
                    LDEBUG( plog, "Starting the FMT; output at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_start ) ) break;
                    a_ctx.f_first_packet_after_start = true;
                }
                if( a_ctx.f_in_command == stream::s_run )
                {
                    t_freq_data = in_stream< 0 >().data();
                    t_trigger_flag = out_stream< 0 >().data();

                    LTRACE( plog, "Considering frequency data:  chan = " << t_freq_data->get_digital_id() <<
                           "  time = " << t_freq_data->get_unix_time() <<
                           "  id = " << t_freq_data->get_pkt_in_session() <<
                           "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                           "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );
                    try
                    {
                        t_array_size = t_freq_data->get_array_size();
                        t_loop_lower_limit = f_n_excluded_bins;
                        t_loop_upper_limit = t_array_size - f_n_excluded_bins;
                        LDEBUG( plog, "Array size: "<<t_array_size );
                        LDEBUG( plog, "Looping from "<<t_loop_lower_limit<<" to "<<t_loop_upper_limit-1 );

                        if( a_ctx.f_first_packet_after_start )
                        {
                            if ( t_mask_buffer.size() != t_freq_data->get_array_size() )
                            {
                                throw psyllid::error() << "Frequency mask is not the same size as frequency data array";
                            }
                            if ( t_mask2_buffer.size() != t_freq_data->get_array_size() )
                            {
                                throw psyllid::error() << "Frequency mask2 is not the same size as frequency data array";
                            }
                            a_ctx.f_first_packet_after_start = false;
                        }

                        t_trigger_flag->set_flag( false );
                        t_trigger_flag->set_high_threshold( false );
                        t_trigger_flag->set_id( t_freq_data->get_pkt_in_session() );

                        for( unsigned i_bin = t_loop_lower_limit; i_bin < t_loop_upper_limit; ++i_bin )
                        {
                            t_real = t_freq_data->get_array()[ i_bin ][ 0 ];
                            t_imag = t_freq_data->get_array()[ i_bin ][ 1 ];
                            t_power_amp = t_real*t_real + t_imag*t_imag;

                            if(  t_power_amp >= t_mask2_buffer[ i_bin ] )
                            {
                                t_trigger_flag->set_flag( true );
                                t_trigger_flag->set_high_threshold( true );
                                LDEBUG( plog, "Data " << t_trigger_flag->get_id() << " [bin " << i_bin <<
                                       "] resulted in flag <" << t_trigger_flag->get_flag() << ">" << '\n' <<
                                       "\tdata: " << t_power_amp << ";  mask2: " << t_mask2_buffer[ i_bin ] );
                                break;
                            }
                            else if( t_power_amp >= t_mask_buffer[ i_bin ] )
                            {
                                t_trigger_flag->set_flag( true );
                                t_trigger_flag->set_high_threshold( false );
                                LTRACE( plog, "Data id <" << t_trigger_flag->get_id() << "> [bin " << i_bin <<
                                       "] resulted in flag <" << t_trigger_flag->get_flag() << ">" << '\n' <<
                                       "\tdata: " << t_power_amp << ";  mask1: " << t_mask_buffer[ i_bin ] );
                            }
                        }

#ifndef NDEBUG
                        if( ! t_trigger_flag->get_flag() )
                        {
                            LTRACE( plog, "Data id <" << t_trigger_flag->get_id() << "> resulted in flag <" <<
                                   t_trigger_flag->get_flag() << ">");
                        }
#endif

                        LTRACE( plog, "FMT writing data to output stream at index " << out_stream< 0 >().get_current_index() );
                        if( ! out_stream< 0 >().set( stream::s_run ) )
                        {
                            LERROR( plog, "Exiting due to stream error" );
                            throw midge::node_nonfatal_error() << "Stream error while applying threshold";
                        }
                    }
                    catch( error& e )
                    {
                        LERROR( plog, "Exiting due to error while processing frequency data: " << e.what() );
                        throw;
                    }

                }
                else if( a_ctx.f_in_command == stream::s_stop )
                {
                    LDEBUG( plog, "FMT is stopping at stream index " << out_stream< 0 >().get_current_index() );
                    if( ! out_stream< 0 >().set( stream::s_stop ) ) break;
                }
                else if( a_ctx.f_in_command == stream::s_exit )
                {
                    LDEBUG( plog, "FMT is exiting at stream index " << out_stream< 0 >().get_current_index() );
                    out_stream< 0 >().set( stream::s_exit );
                    break;
                }

                a_ctx.f_in_command = in_stream< 0 >().get();
                LTRACE( plog, "FMT (apply-threshold) reading stream at index " << in_stream< 0 >().get_current_index() );

            } // while( ! is_canceled() && ! f_break_exe_func.load() )

            LDEBUG( plog, "FMT has exited the apply-two-threshold while loop; possible reasons: is_canceled() = " <<
                   is_canceled() << "; f_break_exe_func.load() = " << f_break_exe_func.load() );
            if( f_break_exe_func.load() )
            {
                LINFO( plog, "FMT is switching exe while loops" );
                return;
            }
            else
            {
                LINFO( plog, "FMT is exiting" );
            }

            LDEBUG( plog, "Stopping output stream" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

            return;
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
            a_node->set_threshold_ampl_snr( a_config["threshold-ampl-snr"]().as_double() );
        }
        if( a_config.has( "threshold-power-snr" ) )
        {
            a_node->set_threshold_snr( a_config["threshold-power-snr"]().as_double() );
        }
        if( a_config.has( "threshold-power-snr-high" ) )
        {
            a_node->set_threshold_snr_high( a_config["threshold-power-snr-high"]().as_double() );
        }
        if( a_config.has( "threshold-power-sigma" ) )
        {
            a_node->set_threshold_sigma( a_config["threshold-power-sigma"]().as_double() );
        }
        if( a_config.has( "threshold-power-sigma-high" ) )
        {
            a_node->set_threshold_sigma_high( a_config["threshold-power-sigma-high"]().as_double() );
        }
        if( a_config.has( "threshold-db" ) )
        {
            a_node->set_threshold_dB( a_config["threshold-db"]().as_double() );
        }
        if( a_config.has( "trigger-mode" ) )
        {
            a_node->set_trigger_mode( a_config["trigger-mode"]().as_string() );
        }
        if( a_config.has( "threshold-type" ) )
        {
            a_node->set_threshold_type( a_config["threshold-type"]().as_string() );
        }
        if( a_config.has( "n-excluded-bins" ))
        {
            a_node->set_n_excluded_bins( a_config["n-excluded-bins"]().as_uint() );
        }
        if( a_config.has( "mask-configuration" ) )
        {
            const scarab::param t_mask_config = a_config["mask-configuration"];
            if ( t_mask_config.is_value() )
            {
                scarab::param_translator t_param_translator = scarab::param_translator();
                scarab::param_ptr_t t_file_param = t_param_translator.read_file( t_mask_config.as_value().as_string() );
                if ( t_file_param->is_node() )
                {
                    a_node->set_mask_parameters_from_node( t_file_param->as_node() );
                }
                else
                {
                    throw psyllid::error() << "mask file must be a node";
                }
            }
            else if ( t_mask_config.is_node() )
            {
                a_node->set_mask_parameters_from_node( t_mask_config.as_node() );
            }
            else
            {
                throw psyllid::error() << "invalid config: mask-configuration must be a file path or a node with mask and mask-data";
            }
        }

        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        return;
    }

    void frequency_mask_trigger_binding::do_dump_config( const frequency_mask_trigger* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for frequency_mask_trigger" );
        a_config.add( "n-packets-for-mask", a_node->get_n_packets_for_mask() );
        a_config.add( "n-spline-points", a_node->get_n_spline_points() );
        a_config.add( "length", a_node->get_length() );
        a_config.add( "trigger-mode", a_node->get_trigger_mode_str() );
        a_config.add( "threshold-type", a_node->get_threshold_type_str() );
        a_config.add( "n-excluded-bins", a_node->get_n_excluded_bins() );

        // get threshold values corresponding only to the configured threshold type
        switch ( a_node->get_threshold_type() )
        {
            case frequency_mask_trigger::threshold_t::snr:
                a_config.add( "threshold-power-snr", a_node->get_threshold_snr() );
                a_config.add( "threshold-power-snr-high", a_node->get_threshold_snr_high() );
                break;
            case frequency_mask_trigger::threshold_t::sigma:
                a_config.add( "threshold-power-sigma", a_node->get_threshold_sigma() );
                a_config.add( "threshold-power-sigma-high", a_node->get_threshold_sigma_high() );
                break;
        }
        //TODO the vectors of average and variance data are large, should add logic to export them to a file
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
                a_node->write_mask( a_args.get_value( "filename", "fmt_mask.yaml" ) );
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
