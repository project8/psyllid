#ifndef PSYLLID_UDP_SERVER_FPA_HH_
#define PSYLLID_UDP_SERVER_FPA_HH_

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
     @class udp_server_fpa
     @author N. S. Oblath

     @brief A UDP server to receive packets via the psyllid-based fast-packet acquisition

     @details
     Configuration values are passed to the constructor.  Passing values is optional; if nullptr is passed, default values below are used.

     Use of this server will require that the executable is run with root privileges.

     This server is only available while using Linux, as it uses low-level networking interfaces.

     Available configuration values:
     - 'interface": string -- Network interface used to receive packets (default: "eth1")
     - "port": int -- UDP port used to listen for packets (default: 5876)
     //- "timeout-sec": uint -- UDP socket timeout in seconds; set to 0 to have no timeout
     - "buffer-size": uint -- Number of packet slots to assign to the buffer that this server reads from
    */
    class udp_server_fpa : public udp_server
    {
        public:
            udp_server_fpa( scarab::param_node* a_config );
            virtual ~udp_server_fpa();

            void activate();

            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = udp_server::f_last_errno );

            void deactivate();

        private:
            std::string f_interface;

            pb_iterator f_iterator;

            static int f_last_errno;
    };

}

#endif /* PSYLLID_UDP_SERVER_FPA_HH_ */
