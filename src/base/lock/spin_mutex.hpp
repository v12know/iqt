//
// Created by carl on 17-11-20.
//

#ifndef _IQT_BASE_LOCK_SPIN_MUTEX_H_
#define _IQT_BASE_LOCK_SPIN_MUTEX_H_

#include <atomic>
#include "base/raii.hpp"

namespace base {

    namespace lock {
        class SpinMutex {
            std::atomic_flag flag = ATOMIC_FLAG_INIT;
        public:
            SpinMutex() = default;

            SpinMutex(const SpinMutex &) = delete;

            SpinMutex &operator=(const SpinMutex &) = delete;

            void lock() {
                while (flag.test_and_set(std::memory_order_acquire));
            }

            void unlock() {
                flag.clear(std::memory_order_release);
            }
            // 将读取锁的申请和释放动作封装为raii对象，自动完成加锁和解锁管理
            raii guard() const noexcept {
                return make_raii(*this, &SpinMutex::unlock, &SpinMutex::lock);
            }

            //这里auto xxx -> xxx 的句法使用用了C++11的"追踪返回类型"特性，将返回类型后置，
            //使用decltype关键字推导出返回类型
            static auto
            make_guard(SpinMutex &lock) -> decltype(base::make_raii(lock, &SpinMutex::unlock, &SpinMutex::lock, true)) {
                return base::make_raii(lock, &SpinMutex::unlock, &SpinMutex::lock, true);
            }
        };
    }
}
#endif //_IQT_BASE_LOCK_SPIN_MUTEX_H_
