#ifndef PSYLLID_UDP_SERVER_HH_
#define PSYLLID_UDP_SERVER_HH_

#include "cancelable.hh"

#include <mutex>
#include <sys/types.h>

namespace psyllid
{

    class udp_server : public scarab::cancelable
    {
        public:
            udp_server();
            virtual ~udp_server();

            virtual void activate() = 0;

            virtual void execute() = 0;

            virtual void reset_read() = 0;
            virtual ssize_t get_next_packet( char* a_message, size_t a_size ) = 0;

            virtual void deactivate() = 0;

        protected:
            static int f_last_errno;
    };

}

#endif /* PSYLLID_UDP_SERVER_HH_ */
