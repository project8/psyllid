#include "udp_server_buffered.hh"

#include "fast_packet_acq_manager.hh"
#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

namespace psyllid
{
    LOGGER( plog, "udp_server_buffered" );

    udp_server_buffered::udp_server_buffered( scarab::param_node* a_config ) :
            f_iterator()
    {
        // default values
        int t_port = 23530;
        unsigned t_timeout_sec = 1;
        std::string t_net_interface( "eth0" );
        unsigned t_buffer_size = 100;

        if( a_config != nullptr )
        {
            a_config->get_value( "port", t_port );
            t_timeout_sec = a_config->get_value( "timeout-sec", t_timeout_sec );
            t_net_interface = a_config->get_value( "net-interface", t_net_interface );
            t_buffer_size = a_config->get_value( "buffer-size", t_buffer_size );
        }

        fast_packet_acq_manager* t_pem = fast_packet_acq_manager::get_instance();
        if( ! t_pem->activate_port( t_net_interface, t_port, f_iterator, t_buffer_size ) )
        {
            LERROR( plog, "Port activation failed" );
            throw error() << "[udp_server_buffered] Unable to activate port <" << t_port << "> on network interface <" << t_net_interface << ">";
        }
    }

    udp_server_buffered::~udp_server_buffered()
    {
    }

    ssize_t udp_server_buffered::recv( char* a_message, size_t a_size, int flags, int& ret_errno )
    {

    }

}
