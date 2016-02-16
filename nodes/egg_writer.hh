/*
 * egg_writer.hh
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#ifndef NODES_EGG_WRITER_HH_
#define NODES_EGG_WRITER_HH_

#include "trigger_flag.hh"
#include "time_data.hh"

#include "consumer.hh"

namespace psyllid
{

    class egg_writer :
            public midge::_consumer< egg_writer, typelist_2( time_data, trigger_flag ) >
    {
        public:
            egg_writer();
            virtual ~egg_writer();

        public:

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();

    };

} /* namespace psyllid */

#endif /* NODES_EGG_WRITER_HH_ */
