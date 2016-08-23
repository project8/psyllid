#include "udp_server_buffered.hh"

namespace psyllid
{
    udp_server_buffered::udp_server_buffered( packet_distributor* a_packet_dist, int a_port, unsigned a_timeout_sec )
    {
    }

    udp_server_buffered::~udp_server_buffered()
    {
    }

    ssize_t udp_server_buffered::recv( char* a_message, size_t a_size, int flags, int& ret_errno )
    {

    }

}
