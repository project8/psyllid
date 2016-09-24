/*
 * tf_roach_monitor.hh
 *
 *  Created on: Sept 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_TF_ROACH_MONITOR_HH_
#define PSYLLID_TF_ROACH_MONITOR_HH_

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
            virtual void execute();
            virtual void finalize();

        private:
            uint64_t f_last_pkt_in_batch;

    };


    class roach_time_monitor_builder : public _node_builder< roach_time_monitor >
    {
        public:
            roach_time_monitor_builder();
            virtual ~roach_time_monitor_builder();

        private:
            virtual void apply_config( roach_time_monitor* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_TF_ROACH_MONITOR_HH_ */
