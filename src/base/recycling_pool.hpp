#pragma once

#include <list>
#include <type_traits>

namespace util {

    /*
     * For saving 'dynamic memory allocation' time,
     * Size of "T" should be pre-allocated.
     */

    template<typename T, uint32_t Size>
    class recycling_pool {
    public:
        static constexpr uint32_t capacity = Size;
        typedef T value_t;
        typedef T *pointer_t;
        typedef std::list<pointer_t> list_t;

    public:
        template<typename... Args>
        explicit recycling_pool(Args &&... __args) {
            for (uint32_t i = 0; i < capacity; ++i)
                pool_.push_back(new value_t(std::forward<Args>(__args)...));
        }

        ~recycling_pool() = default;

        template<typename... Args>
        pointer_t allocate(Args &&... __args) {

            pointer_t ret = nullptr;

            if (pool_.empty()) {
                ret = new(std::nothrow) value_t(std::forward<Args>(__args)...);
                return ret;
            }

            ret = pool_.front();
            pool_.pop_front();
            return ret;
        }

        void deallocate(pointer_t &__location) {
            pool_.push_front(__location);
        }

        std::size_t size() {
            return pool_.size();
        }

    private:
        list_t pool_;
    };

    /*
     * 此内存池的实现可以再优化；因为其作为辅助实现，所以在此不做过多优化和说明。
     */

}