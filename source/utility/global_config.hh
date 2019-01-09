/*
 * global_config.hh
 *
 *  Created on: Aug 25, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_GLOBAL_CONFIG_HH_
#define PSYLLID_GLOBAL_CONFIG_HH_

#include "param.hh"
#include "singleton.hh"

namespace psyllid
{

    class global_config : public scarab::singleton< global_config >
    {
        public:
            void set_config( const scarab::param_node& a_config );

            const scarab::param_node& retrieve() const;

        private:
            scarab::param_node f_config;

        protected:
            friend class scarab::singleton< global_config >;
            friend class scarab::destroyer< global_config >;
            global_config();
            ~global_config();

    };

} /* namespace psyllid */

#endif /* PSYLLID_GLOBAL_CONFIG_HH_ */

