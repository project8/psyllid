/*
 * egg_writer.hh
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_EGG_WRITER_HH_
#define PSYLLID_EGG_WRITER_HH_

#include "../control/control_access.hh"
#include "node_builder.hh"
#include "trigger_flag.hh"
#include "time_data.hh"

#include "consumer.hh"

namespace psyllid
{

    class egg_writer :
            public midge::_consumer< egg_writer, typelist_2( time_data, trigger_flag ) >,
            public control_access
    {
        public:
            egg_writer();
            virtual ~egg_writer();

        public:
            mv_accessible( unsigned, file_size_limit_mb );

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

    };


    class egg_writer_builder : public _node_builder< egg_writer >
    {
        public:
            egg_writer_builder();
            virtual ~egg_writer_builder();

        private:
            virtual void apply_config( egg_writer* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_EGG_WRITER_HH_ */
