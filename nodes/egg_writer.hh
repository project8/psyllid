/*
 * egg_writer.hh
 *
 *  Created on: Dec 30, 2015
 *      Author: nsoblath
 */

#ifndef NODES_EGG_WRITER_HH_
#define NODES_EGG_WRITER_HH_

#include "id_range_event.hh"
#include "time_data.hh"

#include "consumer.hh"

using namespace midge;

namespace psyllid
{

    class egg_writer :
            public _consumer< egg_writer, typelist_2( time_data, id_range_event ) >
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
