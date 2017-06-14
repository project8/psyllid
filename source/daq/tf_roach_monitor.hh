/*
 * tf_roach_monitor.hh
 *
 *  Created on: Sept 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_TF_ROACH_MONITOR_HH_
#define PSYLLID_TF_ROACH_MONITOR_HH_

#include "freq_data.hh"
#include "node_builder.hh"
#include "time_data.hh"

#include "consumer.hh"

namespace psyllid
{

    /*!
     @class roach_time_monitor
     @author N. S. Oblath

     @brief A consumer to check the continuity of the time-packet stream from a ROACH

     @details
     Notifies if the current pkt_in_batch is not the last pkt_in_batch + 1.

     A missed packet that is not recovered will produce one notification (e.g. 1 2 [4] 5 6 ).

     An out-of-order packet will produce three notifications (e.g. 1 2 [4] [3] [5] 6 7 ).

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "roach-time-monitor"

     Available configuration values: (none)

     Output Streams: (none)
    */
    class roach_time_monitor :
            public midge::_consumer< roach_time_monitor, typelist_1( time_data ) >
    {
        public:
            roach_time_monitor();
            virtual ~roach_time_monitor();

        public:

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            uint64_t f_last_pkt_in_batch;
            uint64_t f_packet_count;
            uint64_t f_acquisition_count;

    };


    class roach_time_monitor_binding : public _node_binding< roach_time_monitor, roach_time_monitor_binding >
    {
        public:
            roach_time_monitor_binding();
            virtual ~roach_time_monitor_binding();

        private:
            virtual void do_apply_config( roach_time_monitor* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const roach_time_monitor* a_node, scarab::param_node& a_config ) const;
    };

    /*!
     @class roach_freq_monitor
     @author N. S. Oblath

     @brief A consumer to check the continuity of the freq-packet stream from a ROACH

     @details
     Notifies if the current pkt_in_batch is not the last pkt_in_batch + 1.

     A missed packet that is not recovered will produce one notification (e.g. 1 2 [4] 5 6 ).

     An out-of-order packet will produce three notifications (e.g. 1 2 [4] [3] [5] 6 7 ).

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "roach-freq-monitor"

     Available configuration values: (none)

     Output Streams: (none)
    */
    class roach_freq_monitor :
            public midge::_consumer< roach_freq_monitor, typelist_1( freq_data ) >
    {
        public:
            roach_freq_monitor();
            virtual ~roach_freq_monitor();

        public:

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            uint64_t f_last_pkt_in_batch;
            uint64_t f_packet_count;
            uint64_t f_acquisition_count;

    };


    class roach_freq_monitor_binding : public _node_binding< roach_freq_monitor, roach_freq_monitor_binding >
    {
        public:
            roach_freq_monitor_binding();
            virtual ~roach_freq_monitor_binding();

        private:
            virtual void do_apply_config( roach_freq_monitor* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const roach_freq_monitor* a_node, scarab::param_node& a_config ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_TF_ROACH_MONITOR_HH_ */
