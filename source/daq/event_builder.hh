/*
 * event_builder.hh
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_EVENT_BUILDER_HH_
#define PSYLLID_EVENT_BUILDER_HH_

#include "transformer.hh"

#include "id_range_event.hh"
#include "node_builder.hh"
#include "trigger_flag.hh"

#include <deque>

namespace psyllid
{

    /*!
     @class event_builder
     @author N. S. Oblath

     @brief A transformer that considers a sequence of triggered packets and decides what constitutes a contiguous event

     @details

     Keeps track of the state of the packet sequence (is-triggered, or not).

     When going from untriggered to triggered, adds some number of pretrigger packets.

     Includes a skip tolerance for untriggered packets between two triggered packets.

     Events are built by switching some untriggered packets to triggered packets according to the pretrigger and skip-tolerance parameters.
     Contiguous sequences of triggered packets constitute events.

     Parameter setting is not thread-safe.  Executing is thread-safe.

     Node type: "packet-receiver-socket"

     Available configuration values:
     - "length": uint -- The size of the output buffer
     - "pretrigger": uint -- Number of packets to include in the event before the first triggered packet
     - "skip-tolerance": uint -- Number of untriggered packets to include in the event between two triggered

     Input Streams:
     - 1: trigger_flag

     Output Streams:
     - 0: trigger_flag
    */
    class event_builder :
            public midge::_transformer< event_builder, typelist_1( trigger_flag ), typelist_1( trigger_flag ) >
    {
        public:
            event_builder();
            virtual ~event_builder();

        public:
            mv_accessible( uint64_t, length );
            mv_accessible( uint64_t, pretrigger );
            mv_accessible( uint64_t, skip_tolerance );

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        private:
            void advance_output_stream( trigger_flag* a_write_flag, uint64_t a_id, bool a_trig_flag );

            enum class state_t { filling_pretrigger, untriggered, untriggered_nopt, new_trigger, triggered };
            state_t f_state;

            std::deque< uint64_t > f_pretrigger_buffer;

        public:
            bool is_triggered() const;

            typedef std::deque< uint64_t > pretrigger_buffer_t;
            const pretrigger_buffer_t& pretrigger_buffer() const;

    };


    inline bool event_builder::is_triggered() const
    {
        return f_state == state_t::triggered;
    }

    inline const std::deque< uint64_t >& event_builder::pretrigger_buffer() const
    {
        return f_pretrigger_buffer;
    }


    class event_builder_binding : public _node_binding< event_builder, event_builder_binding >
    {
        public:
            event_builder_binding();
            virtual ~event_builder_binding();

        private:
            virtual void do_apply_config( event_builder* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const event_builder* a_node, scarab::param_node& a_config ) const;
    };


} /* namespace psyllid */

#endif /* PSYLLID_EVENT_BUILDER_HH_ */
