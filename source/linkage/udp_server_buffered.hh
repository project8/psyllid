#ifndef PSYLLID_UDP_SERVER_BUFFERED_HH_
#define PSYLLID_UDP_SERVER_BUFFERED_HH_

#include "udp_server.hh"

#include "packet_buffer.hh"

#include <string>

namespace psyllid
{
    class packet_distributor;

    class udp_server_buffered : public udp_server
    {
        public:
            udp_server_buffered( packet_distributor* a_packet_dist, int a_port, unsigned a_timeout_sec = 0 );
            virtual ~udp_server_buffered();

            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = udp_server::f_last_errno );

        private:
            pb_iterator f_iterator;

            static int f_last_errno;
    };

}

#endif /* PSYLLID_UDP_SERVER_BUFFERED_HH_ */
