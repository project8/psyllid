#ifndef PSYLLID_SERVER_HH_
#define PSYLLID_SERVER_HH_

#include <netinet/in.h>
#include <string>

namespace psyllid
{

    class server
    {
        public:
            server( const int& a_port );
            virtual ~server();

            //ssize_t send( const char* a_message, size_t a_size, int flags = 0, int& ret_errno = f_last_errno );
            ssize_t recv( char* a_message, size_t a_size, int flags = 0, int& ret_errno = f_last_errno );

        private:
            int f_socket;
            sockaddr_in* f_address;

            static int f_last_errno;
    };

}

#endif /* PSYLLID_SERVER_HH_ */
