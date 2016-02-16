/*
 * frequency_mask_trigger.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: nsoblath
 */

#include "frequency_mask_trigger.hh"

#include "psyllid_error.hh"
#include "logger.hh"

#include <cmath>

using midge::stream;

namespace psyllid
{
    REGISTER_NODE( frequency_mask_trigger, "frequency-mask-trigger" );

    LOGGER( plog, "frequency_mask_trigger" );

    frequency_mask_trigger::frequency_mask_trigger() :
            f_length( 10 ),
            f_n_packets_for_mask( 10 ),
            f_threshold_snr( 3. ),
            f_exe_func( &frequency_mask_trigger::exe_add_to_mask ),
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
        DEBUG( plog, "Setting threshold (power via ampl) to " << f_threshold_snr );
        return;
    }

    void frequency_mask_trigger::set_threshold_power_snr( double a_power_snr )
    {
        f_threshold_snr = a_power_snr;
        DEBUG( plog, "Setting threshold (power via power) to " << f_threshold_snr );
        return;
    }

    void frequency_mask_trigger::set_threshold_dB( double a_dB )
    {
        f_threshold_snr = pow( 10, a_dB / 10. );
        DEBUG( plog, "Setting threshold (power via dB) to " << f_threshold_snr );
        return;
    }


    void frequency_mask_trigger::update_trigger_mask()
    {
        std::unique_lock< std::mutex > t_lock( f_mask_mutex );

        f_status = status::mask_update;
        f_exe_func = &frequency_mask_trigger::exe_add_to_mask;

        f_n_summed = 0;
        for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
        {
            f_mask[ i_bin ] = 0.;
        }

        return;
    }

    void frequency_mask_trigger::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void frequency_mask_trigger::execute()
    {
        midge::enum_t t_in_command = stream::s_none;
        freq_data* t_freq_data = nullptr;
        trigger_flag* t_trigger_flag = nullptr;

        while( true )
        {
            t_in_command = in_stream< 0 >().get();

            t_freq_data = in_stream< 0 >().data();
            t_trigger_flag = out_stream< 0 >().data();

            if( t_in_command == stream::s_start )
            {
                DEBUG( plog, "Starting the FMT output" );
                out_stream< 0 >().set( stream::s_start );
                continue;
            }

            if( t_in_command == stream::s_run )
            {
                DEBUG( plog, "Considering frequency data:  chan = " << t_freq_data->get_digital_id() <<
                       "  time = " << t_freq_data->get_unix_time() <<
                       "  id = " << t_freq_data->get_pkt_in_batch() <<
                       "  freqNotTime = " << t_freq_data->get_freq_not_time() <<
                       "  bin 0 [0] = " << (unsigned)t_freq_data->get_array()[ 0 ][ 0 ] );
                (this->*f_exe_func)( t_freq_data, t_trigger_flag );
                continue;
            }

            if( t_in_command == stream::s_stop )
            {
                out_stream< 0 >().set( stream::s_stop );
                continue;
            }

            if( t_in_command == stream::s_exit )
            {
                out_stream< 0 >().set( stream::s_exit );
                break;
            }
        }
    }

    void frequency_mask_trigger::exe_add_to_mask( freq_data* a_freq_data, trigger_flag* )
    {
        static unsigned s_array_size = f_mask.size();

        f_mask_mutex.lock();

        double t_real, t_imag;
        for( unsigned i_bin = 0; i_bin < s_array_size; ++i_bin )
        {
            t_real = a_freq_data->get_array()[ i_bin ][ 0 ];
            t_imag = a_freq_data->get_array()[ i_bin ][ 1 ];
            f_mask[ i_bin ] = f_mask[ i_bin ] + t_real*t_real + t_imag*t_imag;
#ifndef NDEBUG
            if( i_bin < 5 )
            {
                WARN( plog, "Bin " << i_bin << " -- real = " << t_real << ";  imag = " << t_imag << ";  value = " << t_real*t_real + t_imag*t_imag <<
                        ";  mask = " << f_mask[ i_bin ] );
            }
#endif
        }

        ++f_n_summed;
        DEBUG( plog, "Added data to frequency mask; mask now has " << f_n_summed << " packets" );

        if( f_n_summed == f_n_packets_for_mask )
        {
            calculate_mask();
        }

        f_mask_mutex.unlock();

        return;
    }

    void frequency_mask_trigger::exe_apply_threshold( freq_data* a_freq_data, trigger_flag* a_trigger_flag )
    {
        static unsigned s_array_size = f_mask.size();

        DEBUG( plog, "Checking data against mask" );

        f_mask_mutex.lock();

        double t_real, t_imag;
        for( unsigned i_bin = 0; i_bin < s_array_size; ++i_bin )
        {
            t_real = a_freq_data->get_array()[ i_bin ][ 0 ];
            t_imag = a_freq_data->get_array()[ i_bin ][ 1 ];
#ifndef NDEBUG
            if( i_bin < 5 )
            {
                WARN( plog, "Bin " << i_bin << " -- real = " << t_real << ";  imag = " << t_imag << ";  value = " << t_real*t_real + t_imag*t_imag <<
                        ";  mask = " << f_mask[ i_bin ] );
            }
#endif
            if( t_real*t_real + t_imag*t_imag >= f_mask[ i_bin ] )
            {
                a_trigger_flag->set_flag( true );
                a_trigger_flag->set_id( a_freq_data->get_pkt_in_batch() );
                DEBUG( plog, "Data " << a_trigger_flag->get_id() << " [bin " << i_bin << "] resulted in flag <" << a_trigger_flag->get_flag() << ">" << '\n' <<
                       "\tdata: " << t_real*t_real + t_imag*t_imag << ";  mask: " << f_mask[ i_bin ] );
                break;
            }
        }

        f_mask_mutex.unlock();

        out_stream< 0 >().set( stream::s_run );
        return;
    }

    void frequency_mask_trigger::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }

    void frequency_mask_trigger::calculate_mask()
    {
        DEBUG( plog, "Calculating frequency mask" );

        f_status = status::triggering;
        f_exe_func = &frequency_mask_trigger::exe_apply_threshold;

        double t_multiplier = f_threshold_snr / (double)f_n_summed;
        for( unsigned i_bin = 0; i_bin < f_mask.size(); ++i_bin )
        {
#ifndef NDEBUG
            if( i_bin < 5 )
            {
                WARN( plog, "Bin " << i_bin << " -- threshold snr = " << f_threshold_snr << ";  n_summed = " << f_n_summed << ";  multiplier = " << t_multiplier <<
                        ";  summed value = " << f_mask[ i_bin ] << ";  final mask = " << f_mask[ i_bin ] * t_multiplier );
            }
#endif
            f_mask[ i_bin ] = f_mask[ i_bin ] * t_multiplier;
        }

        return;
    }


} /* namespace psyllid */
