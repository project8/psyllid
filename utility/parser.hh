#ifndef PSYLLID_PARSER_HH_
#define PSYLLID_PARSER_HH_

#include "param.hh"

#include <string>

namespace psyllid
{

    class parser : public scarab::param_node
    {
        public:
            parser( int an_argc, char** an_argv );
            virtual ~parser();

            void parse( int an_argc, char** an_argv );
    };

} /* namespace psyllid */

#endif /* PSYLLID_PARSER_HH_ */
