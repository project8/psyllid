/*
 * message_relayer.hh
 *
 *  Created on: Jun 28, 2017
 *      Author: obla999
 */

#ifndef PSYLLID_MESSAGE_RELAYER_HH_
#define PSYLLID_MESSAGE_RELAYER_HH_

#include "relayer.hh"

#include "singleton.hh"

namespace scarab
{
    class param_node;
}

namespace psyllid
{

    class message_relayer : public dripline::relayer, public scarab::singleton< message_relayer >
    {
        public:
            message_relayer( const scarab::param_node* a_config = nullptr );
            virtual ~message_relayer();

        public:
            void slack_notice( const std::string& a_msg_text ) const;
            void slack_warn( const std::string& a_msg_text ) const;
            void slack_error( const std::string& a_msg_text ) const;
            void slack_critical( const std::string& a_msg_text ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_MESSAGE_RELAYER_HH_ */
