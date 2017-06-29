/*
 * message_relayer.cc
 *
 *  Created on: Jun 28, 2017
 *      Author: obla999
 */

#include "message_relayer.hh"

#include "param.hh"

namespace psyllid
{

    message_relayer::message_relayer( const scarab::param_node* a_config ) :
            dripline::relayer( a_config )
    {
    }

    message_relayer::~message_relayer()
    {
    }

    void message_relayer::slack_info( const std::string& a_msg_text ) const
    {
        scarab::param_node* t_msg = new scarab::param_node();
        t_msg->add( "message", new scarab::param_value( a_msg_text ) );
        dripline::relayer::send_async( dripline::msg_alert::create( t_msg, "status_message.info.psyllid" ) );
        return;
    }

    void  message_relayer::slack_warn( const std::string& a_msg_text ) const
    {
        scarab::param_node* t_msg = new scarab::param_node();
        t_msg->add( "message", new scarab::param_value( a_msg_text ) );
        send_async( dripline::msg_alert::create( t_msg, "status_message.warning.psyllid" ) );
        return;
    }

    void  message_relayer::slack_error( const std::string& a_msg_text ) const
    {
        scarab::param_node* t_msg = new scarab::param_node();
        t_msg->add( "message", new scarab::param_value( a_msg_text ) );
        send_async( dripline::msg_alert::create( t_msg, "status_message.error.psyllid" ) );
        return;
    }

    void  message_relayer::slack_critical( const std::string& a_msg_text ) const
    {
        scarab::param_node* t_msg = new scarab::param_node();
        t_msg->add( "message", new scarab::param_value( a_msg_text ) );
        send_async( dripline::msg_alert::create( t_msg, "status_message.critical.psyllid" ) );
        return;
    }


} /* namespace psyllid */
