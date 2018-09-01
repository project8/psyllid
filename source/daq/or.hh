/*
 * or_node.hh
 *
 *  Created on: Aug 31, 2018
 *      Author: cclaessens
 */

#ifndef PSYLLID_OR_NODE_HH_
#define PSYLLID_OR_NODE_HH_

#include "transformer.hh"

#include "id_range_event.hh"
#include "node_builder.hh"
#include "trigger_flag.hh"

#include <boost/circular_buffer.hpp>

namespace psyllid
{

   /*!
     @class or
     @author C. Claessens

     @brief Combines 2 trigger flags using or-logic

     @details

     Node type: "packet-receiver-socket"

     Available configuration values:

     Input Streams:
     - 0: trigger_flag
     - 1: trigger_flag

     Output Streams:
     - 0: trigger_flag
    */
    class or_node :
            public midge::_transformer< or_node, typelist_2( trigger_flag, trigger_flag ), typelist_1( trigger_flag ) >
    {
        public:
            or_node();
            virtual ~or_node();

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();
    };

    class or_node_binding : public _node_binding< or_node, or_node_binding >
    {
        public:
            or_node_binding();
            virtual ~or_node_binding();

        private:
            virtual void do_apply_config( or_node* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const or_node* a_node, scarab::param_node& a_config ) const;
    };


} /* namespace psyllid */

#endif /* PSYLLID_OR_HH_ */
