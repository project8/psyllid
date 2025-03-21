/*
 * roach_time_data.hh
 *
 *  Created on: Mar 25, 2025
 *      Author: pkolbeck
 */

 #ifndef PSYLLID_ROACH_TIME_DATA_HH_
 #define PSYLLID_ROACH_TIME_DATA_HH_
 
 #include "roach_packet.hh"
 #include "iq_time_data.hh"
 
 #include "member_variables.hh"
 
 
 namespace psyllid
 {
 
     class roach_time_data : public roach_packet_data, public iq_time_data
     {
         public:
             roach_time_data();
             virtual ~roach_time_data();
 
     };
 
 
 } /* namespace psyllid */
 
 #endif /* PSYLLID_TIME_DATA_HH_ */
 