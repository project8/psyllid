/*
 * frequency_mask_trigger.hh
 *
 *  Created on: Feb 8, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_FREQUENCY_MASK_TRIGGER_HH_
#define PSYLLID_FREQUENCY_MASK_TRIGGER_HH_

#include "transformer.hh"

#include "freq_data.hh"
#include "node_builder.hh"
#include "trigger_flag.hh"

#include "member_variables.hh"

#include <array>
#include <mutex>

namespace psyllid
{

    /*!
     @class frequency_mask_trigger
     @author N. S. Oblath

     @brief A basic FMT.

     @details
     The FMT has two modes of operation: updating the mask, and triggering.

     When switched to the "updating" mode, the existing mask is erased, and the subsequent spectra that are passed to the FMT are used to
     calculate the new mask.  The number of spectra used for the mask is configurable.  Those spectra are summed together as they arrive.
     Once the appropriate number of spectra have been used, the average value is calculated, and the mask is multiplied by the
     threshold SNR (assuming the data are amplitude values, not power).

     In triggering mode, each arriving spectrum is compared to the mask bin-by-bin.  If a bin crosses the threshold, the spectrum passes
     the trigger and the bin-by-bin comparison is stopped.

     Parameter setting is not thread-safe.  Executing (including switching modes) is thread-safe.

     Node type: "frequency-mask-trigger"

     Available configuration values:
     - "length": uint -- The size of the output data buffer
     - "n-packets-for-mask": uint -- The number of spectra used to calculate the trigger mask
     - "threshold-ampl-snr": float -- The threshold SNR, given as an amplitude SNR
     - "threshold-power-snr": float -- The threshold SNR, given as a power SNR
     - "threshold-dB": float -- The threshold SNR, given as a dB factor

     Input Streams:
     - 0: freq_data (assumed to be frequency data)

     Output Streams:
     - 0: trigger_flag
    */
    class frequency_mask_trigger :
            public midge::_transformer< frequency_mask_trigger, typelist_1( freq_data ), typelist_1( trigger_flag ) >
    {
        public:
            frequency_mask_trigger();
            virtual ~frequency_mask_trigger();

            void set_n_packets_for_mask( unsigned a_n_pkts );

            void set_threshold_ampl_snr( double a_ampl_snr );
            void set_threshold_power_snr( double a_power_snr );
            void set_threshold_dB( double a_dB );

            accessible( uint64_t, length );
            mv_accessible_noset( unsigned, n_packets_for_mask );
            mv_accessible_noset( double, threshold_snr );

        public:
            void update_trigger_mask();

            void initialize();
            void execute( midge::diptera* a_midge = nullptr );
            void finalize();

        private:
            void exe_apply_threshold( freq_data* a_freq_data, trigger_flag* a_trig_flag );
            void exe_add_to_mask( freq_data* a_freq_data, trigger_flag* a_trig_flag );

            void (frequency_mask_trigger::*f_exe_func)( freq_data* a_freq_data, trigger_flag* a_trig_flag );

            void calculate_mask();

        private:
            std::array< double, PAYLOAD_SIZE / 2 > f_mask;
            unsigned f_n_summed;

            std::mutex f_mask_mutex;

        public:
            enum class status
            {
                mask_update,
                triggering
            };

            mv_accessible_noset( status, status );

    };


    class frequency_mask_trigger_builder : public _node_builder< frequency_mask_trigger >
    {
        public:
            frequency_mask_trigger_builder();
            virtual ~frequency_mask_trigger_builder();

        private:
            virtual void apply_config( frequency_mask_trigger* a_node, const scarab::param_node& a_config );
    };


} /* namespace psyllid */

#endif /* PSYLLID_FREQUENCY_MASK_TRIGGER_HH_ */
