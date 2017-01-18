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

    class event_builder :
            public midge::_transformer< event_builder, typelist_1( trigger_flag ), typelist_1( trigger_flag ) >
    {
        public:
            event_builder();
            virtual ~event_builder();

        public:
            accessible( uint64_t, length );
            accessible( uint64_t, pretrigger );
            accessible( uint64_t, skip_tolerance );

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


    class event_builder_builder : public _node_builder< event_builder >
    {
        public:
            event_builder_builder();
            virtual ~event_builder_builder();

        private:
            virtual void apply_config( event_builder* a_node, const scarab::param_node& a_config );
            virtual void dump_config( event_builder* a_node, scarab::param_node& a_config );
    };


} /* namespace psyllid */

#endif /* PSYLLID_EVENT_BUILDER_HH_ */
