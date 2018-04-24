/*
 * streaming_frequency_writer.hh
 *
 *  Created on: April 3, 2018
 *      Author: laroque
 *      (copied from streaming_writer)
 */

#ifndef PSYLLID_STREAMING_FREQUENCY_WRITER_HH_
#define PSYLLID_STREAMING_FREQUENCY_WRITER_HH_

#include "egg_writer.hh"
#include "node_builder.hh"
#include "freq_data.hh"

#include "consumer.hh"

namespace psyllid
{

    /*!
     @class streaming_frequency_writer
     @author B. H. LaRoque

     @brief A consumer to that writes all frequency ROACH packets to an egg file.

     @details

     WARNING! the output of this node is not proper egg file, frequency data is not supported

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "streaming-frequency-writer"

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
     - 0: freq_data

     Output Streams: (none)
    */
    class streaming_frequency_writer :
            public midge::_consumer< streaming_frequency_writer, typelist_1( freq_data ) >,
            public egg_writer
    {
        public:
            streaming_frequency_writer();
            virtual ~streaming_frequency_writer();

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
            unsigned f_last_pkt_in_batch;

            monarch_wrap_ptr f_monarch_ptr;
            unsigned f_stream_no;

    };


    class streaming_frequency_writer_binding : public _node_binding< streaming_frequency_writer, streaming_frequency_writer_binding >
    {
        public:
            streaming_frequency_writer_binding();
            virtual ~streaming_frequency_writer_binding();

        private:
            virtual void do_apply_config( streaming_frequency_writer* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const streaming_frequency_writer* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_STREAMING_FREQUENCY_WRITER_HH_ */
