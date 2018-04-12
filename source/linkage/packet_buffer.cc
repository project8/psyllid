
#include "packet_buffer.hh"

#include "psyllid_error.hh"

#include "logger.hh"

#include <cstdlib>
#include <cstring>
//#include <sstream>


namespace psyllid
{
    LOGGER( plog, "packet_buffer" );


    //*********************
    // packet
    //*********************/

    packet::packet() :
            packet( 0 )
    {}

    packet::packet( size_t a_size ) :
            f_size( 0 ),
            f_status( status::unused ),
            f_bytes( nullptr )
    {
        if( f_size != 0 )
        {
            if( ! reallocate( a_size ) )
            {
                throw error() << "Unable to allocate new packet memory in packet constructor";
            }
        }
    }

    packet::~packet()
    {
        if( f_bytes != nullptr )
        {
            free( f_bytes );
        }
    }

    void packet::memcpy( uint8_t* a_packet, size_t a_size )
    {
        if( a_size != f_size )
        {
            if( ! reallocate( a_size ) )
            {
                throw error() << "Unable to allocate new packet memory";
            }
        }
        ::memcpy( f_bytes, a_packet, f_size);
        return;
    }

    bool packet::reallocate( size_t a_size )
    {
        if( f_bytes != 0 )
        {
            ::free( f_bytes );
            f_bytes = nullptr;
        }
        if( a_size != 0 )
        {
            uint8_t* t_new_bytes = reinterpret_cast< uint8_t* >( malloc( a_size ) );
            if( t_new_bytes == nullptr )
            {
                return false;
            }
            f_bytes = t_new_bytes;
        }
        f_size = a_size;
        return true;
    }


    //*********************
    // packet_buffer
    //*********************/

    packet_buffer::packet_buffer() :
            f_packets( nullptr ),
            f_mutexes( nullptr ),
            f_size( 0 ),
            f_packet_size( 0 )
    {}

    packet_buffer::packet_buffer( size_t a_size, size_t a_packet_size ) :
            f_packets( nullptr ),
            f_mutexes( nullptr ),
            f_size( a_size ),
            f_packet_size( a_packet_size )
    {
        try
        {
            LWARN( plog, "packet_buffer constructor with size " << a_size << " and packet size " << a_packet_size );
            initialize( a_size, a_packet_size );
        }
        catch( error& e )
        {
            throw( e );
        }
    }

    packet_buffer::packet_buffer( const packet_buffer& a_orig ) :
            f_packets( a_orig.f_packets ),
            f_mutexes( a_orig.f_mutexes ),
            f_size( a_orig.f_size ),
            f_packet_size( a_orig.f_packet_size )
    {}

    packet_buffer::packet_buffer( packet_buffer&& a_orig ) :
            f_packets( a_orig.f_packets ),
            f_mutexes( a_orig.f_mutexes ),
            f_size( a_orig.f_size ),
            f_packet_size( a_orig.f_packet_size )
    {
        a_orig.f_packets = nullptr;
        a_orig.f_mutexes = nullptr;
        a_orig.f_size = 0;
        a_orig.f_packet_size = 0;
    }

    packet_buffer::~packet_buffer()
    {
        for( unsigned t_index = 0; t_index < f_size; ++t_index )
        {
            //LWARN( plog, "deleting packet " << t_index );
            delete_packet( t_index );
        }
        delete [] f_packets;
        delete [] f_mutexes;
    }

    void packet_buffer::initialize( size_t a_size, size_t a_packet_size )
    {
        if( f_packets != nullptr )
        {
            throw error() << "Packet buffer is already initialized";
        }

        if( a_size == 0 )
        {
            throw error() << "Cannot have buffer size 0";
        }
        f_size = a_size;
        f_packet_size = a_packet_size;

        f_packets = new packet*[f_size];
        for( unsigned t_index = 0; t_index < f_size; ++t_index )
        {
            f_packets[ t_index ] = new packet( a_packet_size );
            //LWARN( plog, "packet at " << t_index << " now has ptr " << f_packets[t_index] );
        }
        f_mutexes = new std::mutex[f_size];
    }

    void packet_buffer::set_packet( unsigned a_index, packet* a_packet )
    {
        f_mutexes[ a_index ].lock();
        delete f_packets[ a_index ];
        f_packets[ a_index ] = a_packet;
        f_mutexes[ a_index ].unlock();
        return;
    }

    void packet_buffer::print_states()
    {
        std::stringstream pbuff;
        for( unsigned t_index = 0; t_index < f_size; ++t_index )
        {
            pbuff << (unsigned)f_packets[ t_index ]->get_status() << " ";
        }
        LDEBUG( plog, pbuff.str() );
        return;
    }


    //*********************
    // pb_iterator
    //*********************/

    pb_iterator::pb_iterator( const std::string& a_name ) :
            f_name( a_name ),
            f_buffer( nullptr ),
            f_packets( nullptr ),
            f_mutexes( nullptr ),
            f_size( 0 ),
            f_previous_index( 0 ),
            f_current_index( 0 ),
            f_next_index( 0 ),
            f_released( true )
    {}

    pb_iterator::pb_iterator( packet_buffer* a_buffer, const std::string& a_name ) :
            pb_iterator( a_name )
    {
        IT_TIMER_INITIALIZE;

        if( ! attach( a_buffer ) )
        {
            throw error() << "[pb_iterator] Unable to attach pb_iterator to buffer";
        }
    }
    pb_iterator::pb_iterator( const pb_iterator& a_copy )
    {
        f_name = a_copy.f_name;
        f_buffer = a_copy.f_buffer;
        f_packets = a_copy.f_packets;
        f_mutexes = a_copy.f_mutexes;
        f_size = a_copy.f_size;
        f_previous_index = a_copy.f_previous_index;
        f_current_index = a_copy.f_current_index;
        f_next_index = a_copy.f_next_index;
        f_released = a_copy.f_released;
    }
    pb_iterator::pb_iterator( pb_iterator&& a_orig ) :
            f_name( a_orig.f_name ),
            f_buffer( a_orig.f_buffer ),
            f_packets( a_orig.f_packets ),
            f_mutexes( a_orig.f_mutexes ),
            f_size( a_orig.f_size ),
            f_previous_index( a_orig.f_previous_index ),
            f_current_index( a_orig.f_current_index ),
            f_next_index( a_orig.f_next_index ),
            f_released( a_orig.f_released )
    {
        f_buffer = nullptr;
        f_packets = nullptr;
        f_mutexes = nullptr;
        f_size = 0;
        f_previous_index = 0;
        f_current_index = 0;
        f_next_index = 0;
        f_released =true;
    }

    pb_iterator::~pb_iterator()
    {
        if(! f_released ) release();
    }

    bool pb_iterator::operator+()
    {
        IT_TIMER_INCR_TRY_BEGIN;
        if( f_mutexes[ f_next_index ].try_lock() == true )
        {
            IT_TIMER_INCR_LOCKED;
            f_mutexes[ f_current_index ].unlock();
            increment();
            return true;
        }
        IT_TIMER_INCR_TRY_FAIL
        return false;
    }
    void pb_iterator::operator++()
    {
        IT_TIMER_INCR_BEGIN;
        f_mutexes[ f_next_index ].lock();
        IT_TIMER_INCR_LOCKED;
        f_mutexes[ f_current_index ].unlock();
        increment();
        return;
    }

    bool pb_iterator::operator-()
    {
        IT_TIMER_OTHER
        if( f_mutexes[ f_previous_index ].try_lock() == true )
        {
            f_mutexes[ f_current_index ].unlock();
            decrement();
            return true;
        }
        return false;
    }
    void pb_iterator::operator--()
    {
        IT_TIMER_OTHER
        f_mutexes[ f_previous_index ].lock();
        f_mutexes[ f_current_index ].unlock();
        decrement();
        return;
    }

    void pb_iterator::increment()
    {
        f_previous_index++;
        if( f_previous_index == f_size )
        {
            f_previous_index = 0;
        }
        f_current_index++;
        if( f_current_index == f_size )
        {
            f_current_index = 0;
        }
        f_next_index++;
        if( f_next_index == f_size )
        {
            f_next_index = 0;
        }
        return;
    }
    void pb_iterator::decrement()
    {
        if( f_previous_index == 0 )
        {
            f_previous_index = f_size;
        }
        f_previous_index--;
        if( f_current_index == 0 )
        {
            f_current_index = f_size;
        }
        f_current_index--;
        if( f_next_index == 0 )
        {
            f_next_index = f_size;
        }
        f_next_index--;
        return;
    }

    bool pb_iterator::attach( packet_buffer* a_buffer )
    {
        // can only attach if currently released
        if( ! f_released )
        {
            LERROR( plog, "Iterator is already attached to a buffer" );
            return false;
        }

        // cannot attach to a non-existent buffer
        if( a_buffer == nullptr )
        {
            LERROR( plog, "Cannot attach to a non-existent buffer" );
            return false;
        }

        // cannot attach to an uninitialized buffer
        if( a_buffer->size() == 0 )
        {
            LERROR( plog, "Cannot attach to an uninitialized buffer" );
            return false;
        }

        f_buffer = a_buffer;
        f_packets = a_buffer->f_packets;
        f_mutexes = a_buffer->f_mutexes;
        f_size = a_buffer->f_size;
        f_previous_index = f_size;
        f_current_index = 0;
        f_next_index = 1;

        // start out by passing any packets that are currently locked, then lock the first free packet
        IT_TIMER_SET_IGNORE_DECR( (*this) );
        while( f_mutexes[ f_current_index ].try_lock() == false )
        {
            decrement();
        }
        IT_TIMER_UNSET_IGNORE_DECR( (*this) );
        LDEBUG( plog, "pb_iterator " << f_name << " starting at index " << f_current_index );

        f_released = false;

        return true;
    }

    void pb_iterator::release()
    {
        if( f_size == 0 || f_released ) return;
        f_mutexes[ f_current_index ].unlock();
        f_buffer = nullptr;
        f_packets = nullptr;
        f_mutexes = nullptr;
        f_size = 0;
        f_previous_index = 0;
        f_current_index = 0;
        f_next_index = 0;
        f_released = true;
        return;
    }


}
