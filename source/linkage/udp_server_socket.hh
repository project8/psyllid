#ifndef PSYLLID_UDP_SERVER_SOCKET_HH_
#define PSYLLID_UDP_SERVER_SOCKET_HH_

#include "udp_server.hh"

#include <netinet/in.h>

namespace scarab
{
    class param_node;
}

namespace psyllid
{

    /*!
     @class udp_server_socket
     @author N. S. Oblath

     @brief A UDP server to receive packets via the kernel socket tools.

     @details
     Configuration values are passed to the constructor.  Passing values is optional; if nullptr is passed, default values below are used.

     Available configuration values:
     - "port": int -- UDP port used to listen for packets (default: 5876)
     - "timeout-sec": uint -- UDP socket timeout in seconds; set to 0 to have no timeout
    */
    class udp_server_socket : public udp_server
    {
        public:
            udp_server_socket( scarab::param_node* a_config );
            virtual ~udp_server_socket();

            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = udp_server::f_last_errno );

        private:

            int f_socket;
            sockaddr_in* f_address;
    };

}

#endif /* PSYLLID_UDP_SERVER_SOCKET_HH_ */
