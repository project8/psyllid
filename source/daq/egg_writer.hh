/*
 * egg_writer.hh
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_EGG_WRITER_HH_
#define PSYLLID_EGG_WRITER_HH_

#include "control_access.hh"
#include "node_builder.hh"
#include "trigger_flag.hh"
#include "time_data.hh"

#include "consumer.hh"

namespace psyllid
{

    /*!
     @class egg_writer
     @author N. S. Oblath

     @brief A consumer to that writes triggered time ROACH packets to an egg file.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "egg-writer"

     Available configuration values:
     - "file-size-limit-mb": uint -- Not used currently
     - "device": node -- digitizer parameters
       - "bit-depth": uint -- bit depth of each sample
       - "data-type-size": uint -- number of bytes in each sample (or component of a sample for sample-size > 1)
       - "sample-size": uint -- number of components in each sample (1 for real sampling; 2 for IQ sampling)
       - "record-size": uint -- number of samples in each record
       - "acq-rate": uint -- acquisition rate in MHz
       - "v-offset": double -- voltage offset for ADC calibration
       - "v-range": double -- voltage range for ADC calibration
     - "center-freq": double -- the center frequency of the data being digitized
     - "freq-range": double -- the frequency window (bandwidth) of the data being digitized

     ADC calibration: analog (V) = digital * gain + v-offset
                      gain = v-range / # of digital levels

     Input Stream:
     - 0: time_data
     - 1: trigger_flag

     Output Streams: (none)
    */
    class egg_writer :
            public midge::_consumer< egg_writer, typelist_2( time_data, trigger_flag ) >,
            public control_access
    {
        public:
            egg_writer();
            virtual ~egg_writer();

        public:
            mv_accessible( unsigned, file_size_limit_mb );
            mv_referrable( std::string, filename ); /// used if f_daq_control is not set
            mv_referrable( std::string, description ); /// used if f_daq_control is not set

            mv_accessible( unsigned, bit_depth ); // # of bits
            mv_accessible( unsigned, data_type_size ); // # of bytes
            mv_accessible( unsigned, sample_size );  // # of components
            mv_accessible( unsigned, record_size ); // # of samples
            mv_accessible( unsigned, acq_rate ); // MHz
            mv_accessible( double, v_offset ); // V
            mv_accessible( double, v_range ); // V
            mv_accessible( double, center_freq ); // MHz
            mv_accessible( double, freq_range ); // MHz

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

    };


    class egg_writer_binding : public _node_binding< egg_writer, egg_writer_binding >
    {
        public:
            egg_writer_binding();
            virtual ~egg_writer_binding();

        private:
            virtual void do_apply_config( egg_writer* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const egg_writer* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_EGG_WRITER_HH_ */
