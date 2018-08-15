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

#include "logger.hh"

#include <boost/circular_buffer.hpp>

namespace psyllid
{

    LOGGER( eblog_hdr, "event_builder_h" );

    /*!
     @class event_builder
     @author N. S. Oblath, C. Claessens

     @brief A transformer that considers a sequence of triggered packets and decides what constitutes a contiguous event

     @details

     Keeps track of the state of the packet sequence (is-triggered, or not).

     When going from untriggered to triggered, adds some number of pretrigger packets.

     Includes buffers for untriggered packets between triggered packets.

     In untriggered state, two incoming flags are checked.
     If flag from in_stream<1> is true the next state it triggerd.
     If flag from in_stream<0> is true the next state it possibly_triggerd.
     In possibly_triggers state the event builder checks flags on in_stream<1>.

     Contiguous sequences of triggered packets constitute events.

     Parameter setting is not thread-safe.  Executing is thread-safe.

     The configurable value "time-length" in the tf_roach_receiver must be set to a value greater than the individual buffer sizes in the event builder (+5 is advised).
     Otherwise the time domain buffer gets filled and blocks further packet processing.

     Node type: "packet-receiver-socket"

     Available configuration values:
     - "length": uint -- The size of the output buffer
     - "pre_trigger": uint -- Number of packets to include in the event before the first triggered packet
     - "event-length": uint -- Number of untriggered packets to include in the event between minor and major trigger
     - "post-trigger": uint -- Number of untriggered packets to include in the event between major and re-trigger

     Input Streams:
     - 0: trigger_flag
     - 1: trigger_flag
     - 2: trigger_flag

     Output Streams:
     - 0: trigger_flag
    */
    class event_builder :
            public midge::_transformer< event_builder, typelist_3( trigger_flag, trigger_flag, trigger_flag ), typelist_1( trigger_flag ) >
    {
        public:
            typedef boost::circular_buffer< uint64_t > packet_id_buffer_buffer_t;

        public:
            event_builder();
            virtual ~event_builder();

        public:

            mv_accessible( uint64_t, length );
            mv_accessible( uint64_t, pre_trigger_buffer_size );
            mv_accessible( uint64_t, event_buffer_size );
            mv_accessible( uint64_t, post_trigger_buffer_size );


        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

        public:
            bool is_triggered() const;

            const packet_id_buffer_buffer_t& pre_trigger_buffer() const;
            const packet_id_buffer_buffer_t& event_buffer() const;
            const packet_id_buffer_buffer_t& post_trigger_buffer() const;

        private:
            //bool write_output_from_prebuff_front( bool a_flag, trigger_flag* a_data );
            //bool write_output_from_ebuff_front( bool a_flag, trigger_flag* a_data );
            //bool write_output_from_postbuff_front( bool a_flag, trigger_flag* a_data );
            //void advance_output_stream( trigger_flag* a_write_flag, uint64_t a_id, bool a_trig_flag );

            bool write_output_from_buff_front( packet_id_buffer_buffer_t& buffer, bool a_flag, trigger_flag* a_data );
            void move_buffer_content_to_pretrigger( packet_id_buffer_buffer_t& full_buffer, bool surplus_id_flags, trigger_flag* a_data );

            enum class state_t { untriggered, possibly_triggered, triggered, post_triggered };
            state_t f_state;

            packet_id_buffer_buffer_t f_pre_trigger_buffer;
            packet_id_buffer_buffer_t f_event_buffer;
            packet_id_buffer_buffer_t f_post_trigger_buffer;

    };


    inline bool event_builder::is_triggered() const
    {
        return f_state == state_t::triggered;
    }

    inline const event_builder::packet_id_buffer_buffer_t& event_builder::pre_trigger_buffer() const
    {
        return f_pre_trigger_buffer;
    }
    inline const event_builder::packet_id_buffer_buffer_t& event_builder::event_buffer() const
    {
        return f_event_buffer;
    }
    inline const event_builder::packet_id_buffer_buffer_t& event_builder::post_trigger_buffer() const
    {
        return f_post_trigger_buffer;
    }
/*
    inline bool event_builder::write_output_from_prebuff_front( bool a_flag, trigger_flag* a_data )
    {
        a_data->set_id( f_pre_trigger_buffer.front() );
        a_data->set_flag( a_flag );
        LTRACE( eblog_hdr, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
        if( ! out_stream< 0 >().set( midge::stream::s_run ) )
        {
            LERROR( eblog_hdr, "Exiting due to stream error" );
            return false;
        }
        f_pre_trigger_buffer.pop_front();
        return true;
    }
    inline bool event_builder::write_output_from_ebuff_front( bool a_flag, trigger_flag* a_data )
    {
        a_data->set_id( f_event_buffer.front() );
        a_data->set_flag( a_flag );
        LTRACE( eblog_hdr, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
        if( ! out_stream< 0 >().set( midge::stream::s_run ) )
        {
            LERROR( eblog_hdr, "Exiting due to stream error" );
            return false;
        }
        f_event_buffer.pop_front();
        return true;
    }
    inline bool event_builder::write_output_from_postbuff_front( bool a_flag, trigger_flag* a_data )
    {
        a_data->set_id( f_post_trigger_buffer.front() );
        a_data->set_flag( a_flag );
        LTRACE( eblog_hdr, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
        if( ! out_stream< 0 >().set( midge::stream::s_run ) )
        {
            LERROR( eblog_hdr, "Exiting due to stream error" );
            return false;
        }
        f_post_trigger_buffer.pop_front();
        return true;
    }
*/
    inline bool event_builder::write_output_from_buff_front( packet_id_buffer_buffer_t& buffer, bool a_flag, trigger_flag* a_data )
    {
        a_data->set_id( buffer.front() );
        a_data->set_flag( a_flag );
        LTRACE( eblog_hdr, "Event builder writing data to the output stream at index " << out_stream< 0 >().get_current_index() );
        if( ! out_stream< 0 >().set( midge::stream::s_run ) )
        {
            LERROR( eblog_hdr, "Exiting due to stream error" );
            return false;
        }
        buffer.pop_front();
        return true;
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
