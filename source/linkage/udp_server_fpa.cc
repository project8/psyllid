#include "udp_server_fpa.hh"

#include "fpa_factory.hh"
#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

namespace psyllid
{
    LOGGER( plog, "udp_server_buffered" );

    udp_server_fpa::udp_server_fpa( scarab::param_node* a_config ) :
            f_interface( "eth1" ),
            f_iterator()
    {
        // default values
        int t_port = 23530;
        //unsigned t_timeout_sec = 1;
        unsigned t_buffer_size = 100;

        if( a_config != nullptr )
        {
            f_interface = a_config->get_value( "interface", f_interface );
            t_port = a_config->get_value( "port", t_port );
            //t_timeout_sec = a_config->get_value( "timeout-sec", t_timeout_sec );
            t_buffer_size = a_config->get_value( "buffer-size", t_buffer_size );
        }

        fpa_ptr t_fpa = fpa_factory::get_instance()->get_fpa( f_interface );
        t_fpa->initialize();

        if( ! t_fpa->activate_port( t_port, f_iterator, t_buffer_size ) )
        {
            LERROR( plog, "Port activation failed" );
            throw error() << "[udp_server_buffered] Unable to activate port <" << t_port << "> on network interface <" << f_interface << ">";
        }
    }

    udp_server_fpa::~udp_server_fpa()
    {
        f_iterator.release();
    }

    void udp_server_fpa::activate()
    {
        //LDEBUG( plog, "Activating udp_server_fpa" );
        return;
    }

    void udp_server_fpa::execute()
    {
        LDEBUG( plog, "Executing udp_server_fpa" );
        fpa_ptr t_fpa = fpa_factory::get_instance()->get_fpa( f_interface );
        t_fpa->execute();
        return;
    }

    void udp_server_fpa::reset_read()
    {
        // advance f_iterator until it hits the write pointer
        while( +f_iterator ) {}
        return;
    }

    ssize_t udp_server_fpa::get_next_packet( char* a_message, size_t a_size )
    {
        // advance the iterator; blocks until next packet is available
        ++f_iterator;

        // if the IP packet we're on is unused, skip it
        if( f_iterator.is_unused() )
        {
            return 0;
        }

        // read the IP packet
        f_iterator.set_reading();

        if( a_size < f_iterator->get_size() )
        {
            throw error() << "[udp_server_buffered] Message buffer is too small (" << a_size << " bytes, while message is " << f_iterator->get_size() << " bytes)";
        }
        ::memcpy( a_message, f_iterator->ptr(), f_iterator->get_size() );

        f_iterator.set_unused();

        return f_iterator->get_size();
    }

    void udp_server_fpa::deactivate()
    {
        //LDEBUG( plog, "Deactivating udp_server_fpa" );
        return;
    }

    void udp_server_fpa::do_cancellation()
    {
        LDEBUG( plog, "Canceling udp_server_fpa" );
        fpa_ptr t_fpa = fpa_factory::get_instance()->get_fpa( f_interface );
        t_fpa->cancel();
        return;
    }


}
