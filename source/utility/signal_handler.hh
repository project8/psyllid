/*
 * mt_signal_handler.hh
 *
 *  Created on: Dec 3, 2013
 *      Author: nsoblath
 */

#ifndef PSYLLID_SIGNAL_HANDLER_HH_
#define PSYLLID_SIGNAL_HANDLER_HH_

#include <memory>
#include <mutex>
#include <set>

namespace midge
{
    class cancelable;
}

namespace psyllid
{

    class signal_handler
    {
        public:
            signal_handler();
            virtual ~signal_handler();

            void add_cancelable( midge::cancelable* a_cancelable );
            void remove_cancelable( midge::cancelable* a_cancelable );

            void reset();

            static bool got_exit_signal();

            static void handler_cancel_threads( int _ignored );

        private:
            static void print_message();

            typedef std::set< midge::cancelable* > cancelers;
            typedef cancelers::const_iterator cancelers_cit_t;
            typedef cancelers::iterator cancelers_it_t;

            static cancelers f_cancelers;
            static std::mutex f_mutex;

            static bool f_got_exit_signal;

            static bool f_handling_sig_quit;
            static bool f_handling_sig_int;

    };

} /* namespace psyllid */
#endif /* PSYLLID_SIGNAL_HANDLER_HH_ */
