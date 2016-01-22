/*
 * event_builder.hh
 *
 *  Created on: Dec 29, 2015
 *      Author: nsoblath
 */

#ifndef NODES_EVENT_BUILDER_HH_
#define NODES_EVENT_BUILDER_HH_

#include "transformer.hh"

#include "id_range_event.hh"
#include "trigger_flag.hh"

#include <vector>

namespace psyllid
{

    class event_builder :
            public midge::_transformer< event_builder, typelist_1( trigger_flag ), typelist_1( id_range_event ) >
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
            virtual void execute();
            virtual void finalize();

        private:
            enum class state_t { idle, triggered };
            state_t f_state;

            uint64_t f_start_untriggered;
            uint64_t f_end_untriggered;
            std::vector< uint64_t > f_untriggered_buffer;

        public:
            bool is_triggered() const;
            const std::vector< uint64_t >& untriggered_buffer() const;

    };


    inline bool event_builder::is_triggered() const
    {
        return f_state == state_t::triggered;
    }

    inline const std::vector< uint64_t >& event_builder::untriggered_buffer() const
    {
        return f_untriggered_buffer;
    }

} /* namespace psyllid */

#endif /* NODES_EVENT_BUILDER_HH_ */
