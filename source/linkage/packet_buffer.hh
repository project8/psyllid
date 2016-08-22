#ifndef PSYLLID_PACKET_BUFFER_HH_
#define PSYLLID_PACKET_BUFFER_HH_

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
            packet( size_t a_size = 0 );
            ~packet();

            void copy( uint8_t* a_packet, size_t a_size );

            uint8_t* ptr() const;

            size_t size() const;

        private:
            uint8_t* f_bytes;
            size_t f_nbytes;
    };

    inline uint8_t* packet::ptr() const
    {
        return f_bytes;
    }

    inline size_t packet::size() const
    {
        return f_nbytes;
    }


    //*********************
    // buffer
    //*********************

    class buffer
    {
        public:
            friend class iterator;

            buffer( size_t a_size, size_t a_packet_size );
            virtual ~buffer();

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


    inline size_t buffer::size() const
    {
        return f_size;
    }

    inline size_t buffer::packet_size() const
    {
        return f_packet_size;
    }

    inline void buffer::delete_packet( unsigned a_index )
    {
        set_packet( a_index, nullptr );
        return;
    }


    //*********************
    // iterator
    //*********************

    class iterator
    {
        public:
            iterator( buffer* a_buffer, const std::string& a_name = "default" );
            virtual ~iterator();

            const std::string& name() const;

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

            /// remove iterator from buffer
            void release();

        protected:
            iterator();
            iterator( const iterator& );

            std::string f_name;

            buffer* f_buffer;
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


}

#endif /* PSYLLID_PACKET_BUFFER_HH_ */
