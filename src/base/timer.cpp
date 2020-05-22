#include "timer.hpp"


namespace base {

    Timer *Timer::default_timer() {
        static Timer s_timer;
        return &s_timer;
    }
}

