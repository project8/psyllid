/*
 *  slack_relayer.cc
 *
 *  Created on: Feb 18, 2025
 *      Author: N.S. Oblath
 */

#include "slack_relayer.hh"

#include "param.hh"

namespace psyllid
{

    slack_relayer::slack_relayer( const scarab::param_node& a_config, const scarab::authentication& a_auth ) :
            sandfly::message_relayer( a_config, a_auth )
    {}


    void slack_relayer::send_notice( const std::string& a_msg_text ) const
    {
        send_to_slack( a_msg_text, "status_message.notice." );
        return;
    }

    void slack_relayer::send_warn( const std::string& a_msg_text ) const
    {
        send_to_slack( a_msg_text, "status_message.warning." );
        return;
    }

    void slack_relayer::send_error( const std::string& a_msg_text ) const
    {
        send_to_slack( a_msg_text, "status_message.error." );
        return;
    }

    void slack_relayer::send_critical( const std::string& a_msg_text ) const
    {
        send_to_slack( a_msg_text, "status_message.critical." );
        return;
    }

    void slack_relayer::send_to_slack( const std::string& a_msg_text, const std::string& a_rk_root ) const
    {
        if( ! f_use_relayer ) return;
        scarab::param_ptr_t t_msg_ptr( new scarab::param_node() );
        scarab::param_node& t_msg = t_msg_ptr->as_node();
        t_msg.add( "message", scarab::param_value( a_msg_text ) );
        send_async( dripline::msg_alert::create( std::move(t_msg_ptr), a_rk_root + f_queue_name ) );
        return;
    }

} /* namespace psyllid */
