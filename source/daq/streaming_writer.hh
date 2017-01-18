/*
 * streaming_writer.hh
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_STREAMING_WRITER_HH_
#define PSYLLID_STREAMING_WRITER_HH_

#include "control_access.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "consumer.hh"

namespace psyllid
{

    /*!
     @class streaming_writer
     @author N. S. Oblath

     @brief A consumer to that writes all time ROACH packets to an egg file.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "streaming-writer"

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

     ADC calibration: analog (V) = digital * gain + v-offset
                      gain = v-range / # of digital levels

     Input Stream:
     - 0: time_data

     Output Streams: (none)
    */
    class streaming_writer :
            public midge::_consumer< streaming_writer, typelist_1( time_data ) >,
            public control_access
    {
        public:
            streaming_writer();
            virtual ~streaming_writer();

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

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            unsigned f_last_pkt_in_batch;

    };


    class streaming_writer_builder : public _node_builder< streaming_writer >
    {
        public:
            streaming_writer_builder();
            virtual ~streaming_writer_builder();

        private:
            virtual void apply_config( streaming_writer* a_node, const scarab::param_node& a_config );
            virtual void dump_config( streaming_writer* a_node, scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_STREAMING_WRITER_HH_ */
