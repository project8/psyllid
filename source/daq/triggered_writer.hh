/*
 * triggered_writer.hh
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_TRIGGERED_WRITER_HH_
#define PSYLLID_TRIGGERED_WRITER_HH_

#include "egg_writer.hh"
#include "node_builder.hh"
#include "trigger_flag.hh"
#include "time_data.hh"

#include "consumer.hh"

namespace psyllid
{

    /*!
     @class triggered_writer
     @author N. S. Oblath

     @brief A consumer to that writes triggered time ROACH packets to an egg file.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "triggered-writer"

     Available configuration values:
     - "device": node -- digitizer parameters
       - "bit-depth": uint -- bit depth of each sample
       - "data-type-size": uint -- number of bytes in each sample (or component of a sample for sample-size > 1)
       - "sample-size": uint -- number of components in each sample (1 for real sampling; 2 for IQ sampling)
       - "record-size": uint -- number of samples in each record
       - "acq-rate": uint -- acquisition rate in MHz
       - "v-offset": double -- voltage offset for ADC calibration
       - "v-range": double -- voltage range for ADC calibration
     - "center-freq": double -- the center frequency of the data being digitized in Hz
     - "freq-range": double -- the frequency window (bandwidth) of the data being digitized in Hz

     ADC calibration: analog (V) = digital * gain + v-offset
                      gain = v-range / # of digital levels

     Input Stream:
     - 0: time_data
     - 1: trigger_flag

     Output Streams: (none)
    */
    class triggered_writer :
            public midge::_consumer< triggered_writer, typelist_2( time_data, trigger_flag ) >,
            public egg_writer
    {
        public:
            triggered_writer();
            virtual ~triggered_writer();

        public:
            mv_accessible( unsigned, file_num );

            mv_accessible( unsigned, bit_depth ); // # of bits
            mv_accessible( unsigned, data_type_size ); // # of bytes
            mv_accessible( unsigned, sample_size );  // # of components
            mv_accessible( unsigned, record_size ); // # of samples
            mv_accessible( unsigned, acq_rate ); // MHz
            mv_accessible( double, v_offset ); // V
            mv_accessible( double, v_range ); // V
            mv_accessible( double, center_freq ); // Hz
            mv_accessible( double, freq_range ); // Hz

        public:
            virtual void prepare_to_write( monarch_wrap_ptr a_mw_ptr, header_wrap_ptr a_hw_ptr );

            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            struct exe_loop_context
            {
                bool f_is_running;
                bool f_should_exit;
                stream_wrap_ptr f_swrap_ptr;
                monarch3::M3Record* f_record_ptr;
                unsigned f_stream_no;
                bool f_start_file_with_next_data;
                uint64_t f_first_pkt_in_run;
                bool f_is_new_event;
            };

            void exe_loop_not_running( exe_loop_context& a_ctx );
            void exe_loop_is_running( exe_loop_context& a_ctx );

            monarch_wrap_ptr f_monarch_ptr;
            unsigned f_stream_no;
    };


    class triggered_writer_binding : public _node_binding< triggered_writer, triggered_writer_binding >
    {
        public:
            triggered_writer_binding();
            virtual ~triggered_writer_binding();

        private:
            virtual void do_apply_config( triggered_writer* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const triggered_writer* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_TRIGGERED_WRITER_HH_ */
