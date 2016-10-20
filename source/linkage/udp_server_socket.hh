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
        private:
            struct packet_wrapper
            {
                    char* f_message;
                    size_t f_size;
                    bool f_ready;
                    packet_wrapper() :
                        f_message( nullptr ),
                        f_size( 0 ),
                        f_ready( false )
                    {}
            };

        public:
            udp_server_socket( scarab::param_node* a_config );
            virtual ~udp_server_socket();

            void initialize_packet_wrapper( size_t a_size );

            void activate();

            void execute();

            virtual void reset_read();
            virtual ssize_t get_next_packet( char* a_message, size_t a_size );

            void deactivate();

        private:
            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = udp_server::f_last_errno );

            int f_socket;
            sockaddr_in* f_address;

            packet_wrapper f_packet_wrapper;
            size_t f_message_size;
            std::mutex f_pw_mutex;

    };

    inline void udp_server_socket::initialize_packet_wrapper( size_t a_size )
    {
        if( f_packet_wrapper.f_message != nullptr ) delete [] f_packet_wrapper.f_message;
        f_packet_wrapper.f_message = new char[ a_size ];
        f_message_size = a_size;
        return;
    }

    inline void udp_server_socket::reset_read()
    {}

}

#endif /* PSYLLID_UDP_SERVER_SOCKET_HH_ */
