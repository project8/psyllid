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

#include <mutex>
#include <vector>

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

     It is possible to set a second threshold (threshold-power-snr-high).
     A second mask is calculated for this threshold and the incoming spectra are compared to both masks.
     The output trigger flag has an additional variable "high_threshold" which is set true if the higher threshold led to a trigger.
     It is always set to true if only one trigger level is used.

     The mask can be written to a JSON file via the write_mask() function.  The format for the file is:
     {
         "timestamp": "[timestamp]",
         "n-packets": [number of packets averaged],
         "mask": [value_0, value_1, . . . .]
     }

     Parameter setting is not thread-safe.  Executing (including switching modes) is thread-safe.

     Node type: "frequency-mask-trigger"

     Available configuration values:
     - "length": uint -- The size of the output data buffer
     - "n-packets-for-mask": uint -- The number of spectra used to calculate the trigger mask
     - "threshold-ampl-snr": float -- The threshold SNR, given as an amplitude SNR
     - "threshold-power-snr": float -- The threshold SNR, given as a power SNR
     - "threshold-power-snr-high": float -- A second SNR threshold, given as power SNR
     - "threshold-dB": float -- The threshold SNR, given as a dB factor
     - "trigger-mode": string -- The trigger mode, can be set to "single-level-trigger" or "two-level-trigger"
     - "n-spline-points": uint -- The number of points to have in the spline fit for the trigger mask

     Available DAQ commands:
     - "update-mask" (no args) -- Switch the execution mode to updating the trigger mask
     - "apply-trigger" (no args) -- Switch the execution mode to applying the trigger
     - "write-mask" ("filename" string) -- Write the mask in JSON format to the given file

     Input Streams:
     - 0: freq_data (for the apply-trigger mode)
     - 1: freq_data (for the update-mask mode)

     Output Streams:
     - 0: trigger_flag
    */
    class frequency_mask_trigger :
            public midge::_transformer< frequency_mask_trigger, typelist_1( freq_data ), typelist_1( trigger_flag ) >
    {
        public:
            enum class status_t
            {
                mask_update,
                triggering
            };
            enum class trigger_mode_t
            {
                single_level_trigger,
                two_level_trigger
            };
            enum class threshold_type_t
            {
                snr_threshold,
                sigma_threshold
            };

        public:
            frequency_mask_trigger();
            virtual ~frequency_mask_trigger();

            void set_n_packets_for_mask( unsigned a_n_pkts );

            void set_threshold_ampl_snr( double a_ampl_snr );
            void set_threshold_power_snr( double a_power_snr );
            void set_threshold_power_snr_high( double a_power_snr);
            void set_threshold_power_sigma( double a_power_sigma );
            void set_threshold_power_sigma_high( double a_power_sigma);
            void set_threshold_dB( double a_dB );
            void set_trigger_mode( const std::string& trigger_mode );
            void set_threshold_type( const std::string& threshold_type );
            std::string get_trigger_mode_str() const;
            std::string get_threshold_type_str() const;

            mv_accessible( uint64_t, length );
            mv_accessible_noset( unsigned, n_packets_for_mask );
            mv_accessible_noset( double, threshold_snr );
            mv_accessible_noset( double, threshold_snr_high);
            mv_accessible_noset( double, threshold_sigma );
            mv_accessible_noset( double, threshold_sigma_high);
            mv_accessible( threshold_type_t, threshold_type);
            mv_accessible( unsigned, n_spline_points );
            mv_accessible_noset( status_t, status );
            mv_accessible( trigger_mode_t, trigger_mode );

        public:
            void switch_to_update_mask();
            void switch_to_apply_trigger();

            void write_mask( const std::string& a_filename );

            void initialize();
            void execute( midge::diptera* a_midge = nullptr );
            void finalize();

        private:
            struct exe_func_context
            {
                midge::diptera* f_midge;
                bool f_first_packet_after_start;
                midge::enum_t f_in_command;
            };

            void exe_apply_threshold( exe_func_context& a_ctx );
            void exe_apply_two_thresholds( exe_func_context& a_ctx );
            void exe_add_to_mask( exe_func_context& a_ctx );
            void exe_add_to_sigma_mask( exe_func_context& a_ctx );

            void (frequency_mask_trigger::*f_exe_func)( exe_func_context& a_ctx );
            std::mutex f_exe_func_mutex;
            std::atomic< bool > f_break_exe_func;

        private:
            std::vector< double > f_mask;
            std::vector< double > f_mask2;
            std::vector< double > f_mask_data;
            std::vector< double > f_variance_data;
            unsigned f_n_summed;

            std::mutex f_mask_mutex;

    };


    class frequency_mask_trigger_binding : public _node_binding< frequency_mask_trigger, frequency_mask_trigger_binding >
    {
        public:
            frequency_mask_trigger_binding();
            virtual ~frequency_mask_trigger_binding();

        private:
            virtual void do_apply_config( frequency_mask_trigger* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const frequency_mask_trigger* a_node, scarab::param_node& a_config ) const;

            virtual bool do_run_command( frequency_mask_trigger* a_node, const std::string& a_cmd, const scarab::param_node& a_args ) const;
    };


} /* namespace psyllid */

#endif /* PSYLLID_FREQUENCY_MASK_TRIGGER_HH_ */
