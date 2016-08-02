/*
 * packet_processor.h
 *
 *  Created on: Jul 28, 2016
 *      Author: obla999
 */

#ifndef PSYLLID_PACKET_PROCESSOR_H_
#define PSYLLID_PACKET_PROCESSOR_H_

#include "bystander.hh"

namespace psyllid
{

    class packet_processor : public midge::bystander
    {
        public:
            packet_processor();
            virtual ~packet_processor();

        public:
            virtual void initialize();
            virtual void execute();
            virtual void finalize();
    };

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_PROCESSOR_H_ */
