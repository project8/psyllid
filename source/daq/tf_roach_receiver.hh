/*
 * udp_receiver.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_TF_ROACH_RECEIVER_HH_
#define PSYLLID_TF_ROACH_RECEIVER_HH_

#include "freq_data.hh"
#include "memory_block.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "transformer.hh"
#include "shared_cancel.hh"

#include <memory>

namespace scarab
{
    class param_node;
}

namespace psyllid
{
    /*!
     @class tf_roach_receiver
     @author N. S. Oblath

     @brief A transformer to receive raw blocks of memory, parse them, and distribute them as time and frequency ROACH packets.

     @details

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "tf-roach-receiver"

     Available configuration values:
     - "time-length": uint -- The size of the output time-data buffer
     - "freq-length": uint -- The size of the output frequency-data buffer
     - "udp-buffer-size": uint -- The number of bytes in the UDP memory buffer for a single packet; generally this shouldn't be changed and is specified by the ROACH configuration
     - "time-sync-tol": uint -- (currently unused) Tolerance for time synchronization between the ROACH and the server (seconds)
     - "start-paused": bool -- Whether to start execution paused and wait for an unpause command
     - "force-time-first": bool -- If true, when starting ignore f packets before the first t packet

     Available DAQ commands:
     - "freq-only" (no args) -- Switch the execution mode to frequency-only
     - "time-and-freq" (no args) -- Switch the execution mode to time-and-frequency

     Input Stream:
     - 0: memory_block

     Output Streams:
     - 0: time_data
     - 1: freq_data (for time_and_freq mode)
     - 2: freq_data (for freq_only mode)
    */
    class tf_roach_receiver : public midge::_transformer< tf_roach_receiver, typelist_1( memory_block ), typelist_2( time_data, freq_data ) >
    {
        public:
            tf_roach_receiver();
            virtual ~tf_roach_receiver();

        public:
            mv_accessible( uint64_t, time_length );
            mv_accessible( uint64_t, freq_length );
            mv_accessible( uint64_t, udp_buffer_size );
            mv_accessible( unsigned, time_sync_tol );
            mv_accessible( bool, start_paused );
            mv_accessible( bool, force_time_first );
            mv_accessible( unsigned, skip_after_stop );

        public:
            void switch_to_freq_only();
            void switch_to_time_and_freq();

            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            struct exe_func_context
            {
                midge::diptera* f_midge;
                midge::enum_t f_in_command;
                memory_block* f_memory_block;
                time_data* f_time_data;
                freq_data* f_freq_data;
                std::unique_ptr< char[] > f_buffer_ptr;
                size_t f_pkt_size;
            };

            bool exe_time_and_freq( exe_func_context& a_ctx );
            bool exe_freq_only( exe_func_context& a_ctx );

            bool (tf_roach_receiver::*f_exe_func)( exe_func_context& a_ctx );
            std::mutex f_exe_func_mutex;
            std::atomic< bool > f_break_exe_func;

            virtual void do_cancellation();

            bool f_paused;

            uint64_t f_time_session_pkt_counter;
            uint64_t f_freq_session_pkt_counter;

    };

    class tf_roach_receiver_binding : public _node_binding< tf_roach_receiver, tf_roach_receiver_binding >
    {
        public:
            tf_roach_receiver_binding();
            virtual ~tf_roach_receiver_binding();

        private:
            virtual void do_apply_config( tf_roach_receiver* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const tf_roach_receiver* a_node, scarab::param_node& a_config ) const;

            virtual bool do_run_command( tf_roach_receiver* a_node, const std::string& a_cmd, const scarab::param_node& ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_TF_ROACH_RECEIVER_HH_ */
