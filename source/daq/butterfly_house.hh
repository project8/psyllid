/*
 * butterfly_house.hh
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 *
 *  Monarch stages:
 *    - initialized
 *    - preparing
 *    - writing
 *    - finished
 *
 *  Thread safety
 *    Thread-safe operations:
 *      - All butterfly_house function calls
 *      - Initializing an egg file
 *      - Accessing the Monarch header (can only be done by one thread at a time)
 *      - Writing data to a file (handled by HDF5's internal thread safety)
 *      - Finishing an egg file
 *
 *    Non-thread-safe operations:
 *      - Stream function calls are not thread-safe other than HDF5's internal thread safety.
 *        It is highly (highly highly) recommended that you only access a given stream from one thread.
 */

#ifndef PSYLLID_BUTTERFLY_HOUSE_HH_
#define PSYLLID_BUTTERFLY_HOUSE_HH_

#include "monarch3_wrap.hh"

#include "singleton.hh"

namespace psyllid
{

    class butterfly_house : public scarab::singleton< butterfly_house >
    {
        public:

            /// Creates the Monarch3 object for the given filename
            /// Sets the Monarch stage to "initializing"
            monarch_wrap_ptr declare_file( const std::string& a_filename );
/*
            /// Requires that the Monarch stage be "initializing" or "preparing"
            /// If it's in the former, sets the stage to "preparing"
            /// Throws psyllid::error if anything fails
            header_wrap_ptr get_header( const std::string& a_filename ) const;

            /// Requires that the Monarch stage be "preparing" or "writing"
            /// If it's the former, sets the stage to "writing" and writes the Monarch header to the file
            /// Throws psyllid::error if anything fails
            stream_wrap_ptr get_stream( const std::string& a_filename, unsigned a_stream ) const;

            /// Closes the file
            /// Sets the Monarch stage to "finished"
            void write_file( const std::string& a_filename );
*/
            /// Removes the Monarch3 object for the given filename
            void remove_file( const std::string& a_filename );

        private:
            typedef std::map< std::string, monarch_wrap_ptr > bf_map;
            typedef bf_map::const_iterator bf_cit_t;
            typedef bf_map::iterator bf_it_t;
            typedef bf_map::value_type bf_value_t;

            bf_map f_butterflies;
            mutable std::mutex f_house_mutex;


        private:
            friend class scarab::singleton< butterfly_house >;
            friend class scarab::destroyer< butterfly_house >;

            butterfly_house();
            virtual ~butterfly_house();

    };

} /* namespace psyllid */

#endif /* PSYLLID_BUTTERFLY_HOUSE_HH_ */
