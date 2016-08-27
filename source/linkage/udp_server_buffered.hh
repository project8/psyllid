#ifndef PSYLLID_UDP_SERVER_BUFFERED_HH_
#define PSYLLID_UDP_SERVER_BUFFERED_HH_

#include "udp_server.hh"

#include "packet_buffer.hh"

#include <string>


namespace scarab
{
    class param_node;
}

namespace psyllid
{
    class packet_distributor;

    /*!
     @class udp_server_buffered
     @author N. S. Oblath

     @brief A UDP server to receive packets via the psyllid-based fast-packet acquisition

     @details
     Configuration values are passed to the constructor.  Passing values is optional; if nullptr is passed, default values below are used.

     Available configuration values:
     - "net-interface": string -- Network interface name that will receive packets (default: "eth0")
     - "port": int -- UDP port used to listen for packets (default: 5876)
     - "timeout-sec": uint -- UDP socket timeout in seconds; set to 0 to have no timeout
    */
    class udp_server_buffered : public udp_server
    {
        public:
            udp_server_buffered( scarab::param_node* a_config );
            virtual ~udp_server_buffered();

            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = udp_server::f_last_errno );

        private:
            pb_iterator f_iterator;

            static int f_last_errno;
    };

}

#endif /* PSYLLID_UDP_SERVER_BUFFERED_HH_ */
