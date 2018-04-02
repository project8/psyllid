/*
 * frequency_transform.hh
 *
 *  Created on: March 28, 2018
 *      Author: laroque
 */

#ifndef PSYLLID_FREQUENCY_TRANSFORM_HH_
#define PSYLLID_FREQUENCY_TRANSFORM_HH_

#include "freq_data.hh"
//#include "memory_block.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "transformer.hh"
#include "shared_cancel.hh"

//#include <memory>

namespace scarab
{
    class param_node;
}

namespace psyllid
{
    /*!
     @class frequency_transform
     @author B. H. LaRoque

     @brief A transformer to receive time data, compute an FFT, and distribute as time and frequency ROACH packets.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "frequency-transform"

     Available configuration values:
     - "time-length": uint -- The size of the output time-data buffer
     - "freq-length": uint -- The size of the output frequency-data buffer
     - "start-paused": bool -- Whether to start execution paused and wait for an unpause command

     Input Stream:
     - 0: time_data

     Output Streams:
     - 0: time_data
     - 1: freq_data
    */
    class frequency_transform : public midge::_transformer< frequency_transform, typelist_1( time_data ), typelist_2( time_data, freq_data ) >
    {
        private:
            typedef std::map< std::string, unsigned > TransformFlagMap;

        public:
            frequency_transform();
            virtual ~frequency_transform();

        public:
            mv_accessible( uint64_t, time_length );
            mv_accessible( uint64_t, freq_length );
            mv_accessible( unsigned, fft_size ); // I really wish I could get this from the egg header
            //mv_accessible( uint64_t, udp_buffer_size );
            //mv_accessible( unsigned, time_sync_tol );
            mv_accessible( bool, start_paused );
            //mv_accessible( unsigned, skip_after_stop );

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            TransformFlagMap f_transform_flag_map;
            /*struct exe_func_context
            {
                midge::diptera* f_midge;
                midge::enum_t f_in_command;
                memory_block* f_memory_block;
                time_data* f_time_data;
                freq_data* f_freq_data;
                std::unique_ptr< char[] > f_buffer_ptr;
                size_t f_pkt_size;
            };*/

            //bool exe_time_and_freq( exe_func_context& a_ctx );
            //bool exe_freq_only( exe_func_context& a_ctx );

            //bool (tf_roach_receiver::*f_exe_func)( exe_func_context& a_ctx );
            //std::mutex f_exe_func_mutex;
            //std::atomic< bool > f_break_exe_func;

            //virtual void do_cancellation();

            bool f_paused;

            uint64_t f_time_session_pkt_counter;
            uint64_t f_freq_session_pkt_counter;
        private:
            void setup_internal_maps();

    };

    class frequency_transform_binding : public _node_binding< frequency_transform, frequency_transform_binding >
    {
        public:
            frequency_transform_binding();
            virtual ~frequency_transform_binding();

        private:
            virtual void do_apply_config( frequency_transform* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const frequency_transform* a_node, scarab::param_node& a_config ) const;

            //virtual bool do_run_command( frequency_transform* a_node, const std::string& a_cmd, const scarab::param_node& ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_FREQUENCY_TRANSFORM_HH_ */
