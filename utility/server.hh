#ifndef PSYLLID_SERVER_HH_
#define PSYLLID_SERVER_HH_

#include "connection.hh"

#include <netinet/in.h>
#include <string>

namespace psyllid
{

    class server
    {
        public:
            server( const int& a_port );
            virtual ~server();

            connection* get_connection();

        private:
            int f_socket;
            sockaddr_in* f_address;
    };

}

#endif /* PSYLLID_SERVER_HH_ */
