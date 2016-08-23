#ifndef PSYLLID_UDP_SERVER_HH_
#define PSYLLID_UDP_SERVER_HH_

#include <cstdlib>

namespace psyllid
{

    class udp_server
    {
        public:
            udp_server();
            virtual ~udp_server();

            virtual ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = f_last_errno ) = 0;

        protected:
            static int f_last_errno;
    };

}

#endif /* PSYLLID_UDP_SERVER_HH_ */
