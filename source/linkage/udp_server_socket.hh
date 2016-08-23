#ifndef PSYLLID_UDP_SERVER_SOCKET_HH_
#define PSYLLID_UDP_SERVER_SOCKET_HH_

#include "udp_server.hh"

#include <netinet/in.h>

namespace psyllid
{

    class udp_server_socket : public udp_server
    {
        public:
            udp_server_socket( int a_port, unsigned a_timeout_sec = 0 );
            virtual ~udp_server_socket();

            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = udp_server::f_last_errno );

        private:

            int f_socket;
            sockaddr_in* f_address;
    };

}

#endif /* PSYLLID_UDP_SERVER_SOCKET_HH_ */
