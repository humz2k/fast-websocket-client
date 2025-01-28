#ifndef _BENCHMARK_BENCHMARK_HPP_
#define _BENCHMARK_BENCHMARK_HPP_

#include <pthread.h>
#include <stdexcept>

#define MESSAGE "ping"
#define NSENDS 10000

static inline void set_max_priority() {
    pthread_t thread = pthread_self();
    int policy;
    sched_param param = {};
    pthread_getschedparam(thread, &policy, &param);
    int max_priority = sched_get_priority_max(policy);
    if (param.sched_priority != max_priority) {
        param.sched_priority = max_priority;
        if (pthread_setschedparam(thread, policy, &param) != 0) {
            throw std::runtime_error("Failed to set thread policy");
        }
    }
}

#endif // _BENCHMARK_BENCHMARK_HPP_