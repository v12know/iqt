#ifndef _BASE_TIMER_HPP_
#define _BASE_TIMER_HPP_

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <queue>
#include <chrono>
#include <cassert>
#include <iostream>

#include "base/macro.hpp"

namespace base {
    class Timer {
    public:
        typedef std::function<void(void *)> TimerTask;

    private:
        class TimerTaskWrapper {
        public:
            TimerTaskWrapper(long delay, long period, TimerTask task, void *param = nullptr,
                             std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
                    : delay(delay), period(period), task(task), param(param), start_time(std::move(start_time)) {}

            virtual ~TimerTaskWrapper() {}

            void run() {
                task(param);
            }

            long delay;
            long period;
            TimerTask task;
            void *param;
            std::chrono::system_clock::time_point start_time;
        };

        class TimerTaskWrapperCompare {
        public:
            bool operator()(const TimerTaskWrapper &lhs, const TimerTaskWrapper &rhs) const {
                return lhs.start_time + std::chrono::milliseconds(lhs.delay) >
                       rhs.start_time + std::chrono::milliseconds(rhs.delay);
            }
        };

    public:
        static Timer *default_timer();

        Timer() : _stop_flag(false), _looper(std::thread(std::bind(&Timer::run, this))) {}

        virtual ~Timer() {
//            join();
        }

        void schedule(long delay, TimerTask task, void *param = nullptr) {
            assert(delay >= 0);
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _queue.emplace(delay, 0, std::move(task), param);
            if (1 == _queue.size()) {
                lock.unlock();
                _queue_cv.notify_all();
            }
        }

        void schedule(const std::chrono::system_clock::time_point &start_time, TimerTask task, void *param = nullptr) {
            auto now = std::chrono::system_clock::now();
            long delay = std::chrono::duration_cast<std::chrono::milliseconds>(
                    start_time - now).count();
            delay = delay < 0 ? 0 : delay;
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _queue.emplace(delay, 0, task, param, now);
            if (1 == _queue.size()) {
                lock.unlock();
                _queue_cv.notify_all();
            }
        }

        void schedule(long delay, long period, TimerTask task, void *param = nullptr) {
            assert(delay >= 0 && period > 0);
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _queue.emplace(delay, period, std::move(task), param);
            if (1 == _queue.size()) {
                lock.unlock();
                _queue_cv.notify_all();
            }
        }

        void schedule(const std::chrono::system_clock::time_point &start_time, long period, TimerTask task, void *param = nullptr) {
            auto now = std::chrono::system_clock::now();
            long delay = std::chrono::duration_cast<std::chrono::milliseconds>(
                    start_time - now).count();
            delay = delay < 0 ? 0 : delay;
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _queue.emplace(delay, period, std::move(task), param, now);
            if (1 == _queue.size()) {
                lock.unlock();
                _queue_cv.notify_all();
            }
        }

        void stop() {
            _stop_flag = true;
        }

        void join() {
            if (_looper.joinable()) _looper.join();
        }

        void detach() {
            _looper.detach();
        }
        DISALLOW_COPY_AND_ASSIGN(Timer);

    private:

        void run() {
            while (!_stop_flag) {
                std::unique_lock<std::mutex> lock(_queue_mutex);
                while (_queue.empty()) _queue_cv.wait(lock);
                auto task = _queue.top();
                _queue.pop();
                lock.unlock();

                std::unique_lock<std::mutex> lock2(_timeout_mutex);
                auto now = std::chrono::system_clock::now();
                task.delay -= std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - task.start_time).count();
                task.start_time = now;
                auto status = _timeout_cv.wait_for(lock2, std::chrono::milliseconds(task.delay));
                if (status == std::cv_status::no_timeout) {
                    now = std::chrono::system_clock::now();
                    task.delay -= std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - task.start_time).count();
                    task.start_time = now;
                    std::unique_lock<std::mutex> lock(_queue_mutex);
                    _queue.push(std::move(task));
                    continue;
                }
                task.run();
                if (task.period > 0) {
/*                    now = std::chrono::system_clock::now();
                    task.delay = task.period - std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - task.start_time).count();
                    task.start_time = now;*/
                    now = std::chrono::system_clock::now();
                    task.delay = task.delay + task.period - std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - task.start_time).count();
                    task.start_time = now;
                    std::unique_lock<std::mutex> lock(_queue_mutex);
                    _queue.push(std::move(task));
                }
            }
        }

    private:
        std::atomic_bool _stop_flag;
        std::thread _looper;
        std::mutex _queue_mutex;
        std::mutex _timeout_mutex;
        std::condition_variable _queue_cv;
        std::condition_variable _timeout_cv;
        std::priority_queue<TimerTaskWrapper, std::vector<TimerTaskWrapper>, TimerTaskWrapperCompare> _queue;
    };
} /* base  */
#endif /* end of include guard: _BASE_TIMER1_HPP_ */
