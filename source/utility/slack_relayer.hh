/*
 * slack_relayer.hh
 *
 *  Created on: Feb 18, 2025
 *      Author: N.S. Oblath
 */

#ifndef PSYLLID_SLACK_RELAYER_HH_
#define PSYLLID_SLACK_RELAYER_HH_

#include "message_relayer.hh"

#include "param_fwd.hh"

namespace psyllid
{

    class slack_relayer : public sandfly::message_relayer
    {
        public:
            slack_relayer( const scarab::param_node& a_config, const scarab::authentication& a_auth );
            slack_relayer( const slack_relayer& ) = delete;
            slack_relayer( slack_relayer&& ) = default;
            virtual ~slack_relayer() = default;

            slack_relayer& operator=( const slack_relayer& ) = delete;
            slack_relayer& operator=( slack_relayer&& ) = default;

        public:
            void send_notice( const std::string& a_msg_text ) const override;
            void send_warn( const std::string& a_msg_text ) const override;
            void send_error( const std::string& a_msg_text ) const override;
            void send_critical( const std::string& a_msg_text ) const override;

            void send_notice( scarab::param_ptr_t&& a_payload ) const override;
            void send_warn( scarab::param_ptr_t&& a_payload ) const override;
            void send_error( scarab::param_ptr_t&& a_payload ) const override;
            void send_critical( scarab::param_ptr_t&& a_payload ) const override;

        protected:
            void send_to_slack( const std::string& a_msg_text, const std::string& a_rk_root ) const;
            void send_to_slack( scarab::param_ptr_t&& a_payload, const std::string& a_rk_root ) const;
    };

} /* namespace psyllid */

#endif /* PSYLLID_SLACK_RELAYER_HH_ */
