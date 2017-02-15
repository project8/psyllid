/*
 * terminator_freq_data.hh
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_TERMINATOR_FREQ_DATA_HH_
#define PSYLLID_TERMINATOR_FREQ_DATA_HH_

#include "node_builder.hh"

#include "consumer.hh"

#include "freq_data.hh"
#include "time_data.hh"

namespace psyllid
{

    class terminator_time_data :
            public midge::_consumer< terminator_time_data, typelist_1( time_data ) >
    {
        public:
            terminator_time_data();
            virtual ~terminator_time_data();

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

    };

    class terminator_time_data_binding : public _node_binding< terminator_time_data >
    {
        public:
            terminator_time_data_binding();
            virtual ~terminator_time_data_binding();

            node_binding* clone() const;

        private:
            virtual void apply_config( terminator_time_data* a_node, const scarab::param_node& a_config ) const;
            virtual void dump_config( const terminator_time_data* a_node, scarab::param_node& a_config ) const;
    };


    class terminator_freq_data :
            public midge::_consumer< terminator_freq_data, typelist_1( freq_data ) >
    {
        public:
            terminator_freq_data();
            virtual ~terminator_freq_data();

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

    };

    class terminator_freq_data_binding : public _node_binding< terminator_freq_data >
    {
        public:
            terminator_freq_data_binding();
            virtual ~terminator_freq_data_binding();

            node_binding* clone() const;

        private:
            virtual void apply_config( terminator_freq_data* a_node, const scarab::param_node& a_config ) const;
            virtual void dump_config( const terminator_freq_data* a_node, scarab::param_node& a_config ) const;
    };


} /* namespace psyllid */

#endif /* PSYLLID_TERMINATOR_FREQ_DATA_HH_ */
