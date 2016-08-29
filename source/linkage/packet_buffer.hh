#ifndef PSYLLID_PACKET_BUFFER_HH_
#define PSYLLID_PACKET_BUFFER_HH_

#include "member_variables.hh"

#include <inttypes.h>
#include <mutex>
#include <string>

namespace psyllid
{

    //*********************
    // packet
    //*********************

    class packet
    {
        public:
            enum class status
            {
                unused,
                writing,
                written,
                reading
            };

        public:
            packet();
            packet( size_t a_size );
            ~packet();

            void memcpy( uint8_t* a_packet, size_t a_size );

            uint8_t* ptr() const;

            mv_accessible_noset( size_t, size );
            mv_accessible( status, status );

        private:
            friend class pb_iterator;

            uint8_t* f_bytes;
    };

    inline uint8_t* packet::ptr() const
    {
        return f_bytes;
    }


    //*********************
    // packet_buffer
    //*********************

    class packet_buffer
    {
        public:
            friend class pb_iterator;

            packet_buffer();
            packet_buffer( size_t a_size, size_t a_packet_size = 0 );
            virtual ~packet_buffer();

            size_t size() const;
            size_t packet_size() const;

            void set_packet( unsigned a_index, packet* a_packet );
            void delete_packet( unsigned a_index );

            void print_states();

        private:
            packet** f_packets;
            std::mutex* f_mutexes;
            size_t f_size;
            size_t f_packet_size;
    };


    inline size_t packet_buffer::size() const
    {
        return f_size;
    }

    inline size_t packet_buffer::packet_size() const
    {
        return f_packet_size;
    }

    inline void packet_buffer::delete_packet( unsigned a_index )
    {
        set_packet( a_index, nullptr );
        return;
    }


    //*********************
    // pb_iterator
    //*********************

    class pb_iterator
    {
        public:
            pb_iterator();
            pb_iterator( packet_buffer* a_buffer /*, const std::string& a_name = "default"*/ );
            pb_iterator( const pb_iterator& );
            virtual ~pb_iterator();

            //const std::string& name() const;

            /// returns the index of the current packet in the buffer
            unsigned int index() const;
            /// returns a pointer to the current packet
            packet* object();

            /// returns a reference to the current packet
            packet& operator*();
            /// returns a pointer to the current packet
            packet* operator->();

            /// move to the next packet; packets thread if the next packet is locked
            void operator++();
            /// try to move to the next packet; fails if the next packet's mutex is locked
            bool operator+();
            /// move to the previous packet; packets thread if the next packet is locked
            void operator--();
            /// try to move to the previous packet;
            bool operator-();

            /// attach pb_iterator to a buffer
            bool attach( packet_buffer* a_buffer );

            /// remove pb_iterator from buffer
            void release();

            // status access
            bool is_unused() const;
            bool is_writing() const;
            bool is_written() const;
            bool is_reading() const;

            void set_unused();
            void set_writing();
            void set_written();
            void set_reading();

        protected:

            //std::string f_name;

            packet_buffer* f_buffer;
            packet** f_packets;
            std::mutex* f_mutexes;
            unsigned int f_size;

            void increment();
            void decrement();
            unsigned int f_previous_index;
            unsigned int f_current_index;
            unsigned int f_next_index;

            bool f_released;

            //IT_TIMER_DECLARE
    };

    /*
    inline const std::string& pb_iterator::name() const
    {
        return f_name;
    }
    */

    inline unsigned int pb_iterator::index() const
    {
        return f_current_index;
    }
    inline packet* pb_iterator::object()
    {
        return f_packets[ f_current_index ];
    }

    inline packet* pb_iterator::operator->()
    {
        return f_packets[ f_current_index ];
    }
    inline packet& pb_iterator::operator*()
    {
        return *f_packets[ f_current_index ];
    }

    inline bool pb_iterator::is_unused() const
    {
        return f_packets[ f_current_index ]->f_status == packet::status::unused;
    }

    inline bool pb_iterator::is_writing() const
    {
        return f_packets[ f_current_index ]->f_status == packet::status::writing;
    }

    inline bool pb_iterator::is_written() const
    {
        return f_packets[ f_current_index ]->f_status == packet::status::written;
    }

    inline bool pb_iterator::is_reading() const
    {
        return f_packets[ f_current_index ]->f_status == packet::status::reading;
    }

    inline void pb_iterator::set_unused()
    {
        f_packets[ f_current_index ]->f_status = packet::status::unused;
    }

    inline void pb_iterator::set_writing()
    {
        f_packets[ f_current_index ]->f_status = packet::status::writing;
    }

    inline void pb_iterator::set_written()
    {
        f_packets[ f_current_index ]->f_status = packet::status::written;
    }

    inline void pb_iterator::set_reading()
    {
        f_packets[ f_current_index ]->f_status = packet::status::reading;
    }

}

#endif /* PSYLLID_PACKET_BUFFER_HH_ */
