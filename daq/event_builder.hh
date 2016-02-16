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
#include "trigger_flag.hh"

#include <vector>

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
            virtual void execute();
            virtual void finalize();

        private:
            void advance_output_stream( trigger_flag* a_write_flag, uint64_t a_id, bool a_trig_flag );

            enum class state_t { untriggered, triggered };
            state_t f_state;

            const uint64_t f_invalid_id;

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

    inline void event_builder::advance_output_stream( trigger_flag* a_write_flag, uint64_t a_id, bool a_trig_flag )
    {
         a_write_flag->set_id( a_id );
         a_write_flag->set_flag( a_trig_flag );
         out_stream< 0 >().set( midge::stream::s_run );
         return;
    }

} /* namespace psyllid */

#endif /* PSYLLID_EVENT_BUILDER_HH_ */
