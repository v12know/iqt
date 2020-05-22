#ifndef _BASE_REDIS_REDIS_POOL_HPP_
#define _BASE_REDIS_REDIS_POOL_HPP_

#include <list>
#include <mutex>
#include <string>
#include <memory>


namespace cpp_redis {
    class client;
}

namespace base {

    namespace redis {

        class RedisPool {
        public:
            //RedisPool(std::string host, int port, std::string password, int dbIdex);
            RedisPool(std::string host, int port, int db, std::string password) :
                    host_(host), port_(port), db_(db), password_(password),
                    max_idle_time_(3600) {}

            virtual ~RedisPool() { clear(); }

            bool empty() const { return pool_.empty(); }

            std::shared_ptr<cpp_redis::client> grab();

            std::shared_ptr<cpp_redis::client> grab_tl();

            std::shared_ptr<cpp_redis::client> safe_grab();

            virtual void release(cpp_redis::client *pc);

            void remove(const cpp_redis::client *pc);


            void shrink() { clear(false); }

        protected:
            virtual cpp_redis::client *raw_grab();

            void clear(bool all = true);

            virtual cpp_redis::client *create();

            virtual void destroy(cpp_redis::client *c);

            virtual unsigned int max_idle_time() { return max_idle_time_; }

            size_t size() const { return pool_.size(); }

        private:

            struct RedisInfo {
                cpp_redis::client *client_;
                time_t last_used_;
                bool in_use_;

                RedisInfo(cpp_redis::client *c) :
                        client_(c),
                        last_used_(time(0)),
                        in_use_(true) {
                }

                // Strict weak ordering for RedisInfo objects.
                //
                // This ordering defines all in-use connections to be "less
                // than" those not in use.  Within each group, connections
                // less recently touched are less than those more recent.
                bool operator<(const RedisInfo &rhs) const {
                    const RedisInfo &lhs = *this;
                    return lhs.in_use_ == rhs.in_use_ ?
                           lhs.last_used_ < rhs.last_used_ :
                           lhs.in_use_;
                }
            };

            typedef std::list<RedisInfo> PoolT;
            typedef PoolT::iterator PoolIt;

            //// Internal support functions
            cpp_redis::client *find_mru();

            void remove(const PoolIt &it);

            void remove_old_connections();

            static bool ping(cpp_redis::client *c);

            //// Internal data
            PoolT pool_;
            std::mutex mutex_;

            std::string host_;
            int port_;
            int db_;
            std::string password_;
            unsigned int max_idle_time_;
        };
    } /* redis */
}/* base */
#include "redis_pool_impl.hpp"
#endif /* end of include guard: _BASE_REDIS_REDIS_POOL_HPP_ */
