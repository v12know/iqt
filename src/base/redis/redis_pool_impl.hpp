#ifndef _BASE_REDIS_REDIS_POOL_IMPL_HPP_
#define _BASE_REDIS_REDIS_POOL_IMPL_HPP_

#include "redis_pool.hpp"

#include <cstring>
#include <algorithm>
#include <cpp_redis/cpp_redis>
#include <sstream>

#include "base/log.h"
#include "base/exception.h"

namespace base {
    namespace redis {
        template<typename RedisInfoT>
        class TooOld : std::unary_function<RedisInfoT, bool> {
        public:
            TooOld(unsigned int tmax) :
                    min_age_(time(0) - tmax) {
            }

            bool operator()(const RedisInfoT &info) const {
                return !info.in_use_ && info.last_used_ <= min_age_;
            }

        private:
            time_t min_age_;
        };

//RedisPool::RedisPool(std::string host, int port, std::string password, int db_index) :
        //host_(host), port_(port), password_(password), db_index_(db_index),
        //maxIdleTime_(10) { }

        inline std::shared_ptr<cpp_redis::client> RedisPool::grab() {
            std::lock_guard<std::mutex> lock(mutex_);    // ensure we're not interfered with
            std::shared_ptr<cpp_redis::client> ptr(raw_grab(), [&](cpp_redis::client *c) { release(c); });
            return ptr;
        }

        inline std::shared_ptr<cpp_redis::client> RedisPool::grab_tl() {
            static thread_local std::shared_ptr<cpp_redis::client> ptr = grab();
            return ptr;
        }

        inline cpp_redis::client *RedisPool::create() {
            cpp_redis::client *client = new cpp_redis::client();
            client->connect(host_, port_, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
                if (status == cpp_redis::client::connect_state::dropped) {
//                    log_critical("client disconnected from [host={0}][port={1}]", host, port);
                    std::ostringstream oss;
                    oss << "client disconnected from [host=" << "][port=" << port << "]";
                    MY_THROW(base::BaseException, oss.str());
                }
            });

            if (!password_.empty()) {
                auto auth = client->auth(password_);
                client->commit();
                cpp_redis::reply r = auth.get();
                if (r.ko()) {
/*                    log_error("Redis [host={0}][port={1}] authentication failed, error: {2}", host_, port_,
                              r.as_string());*/
                    std::ostringstream oss;
                    oss << "Redis [host=" << host_ << "][port=" << port_ << "] authentication failed, error: " << r.as_string();
                    MY_THROW(base::BaseException, oss.str());
                }
            }
            if (db_ != 0) {
                auto sel = client->select(db_);
                client->commit();
                cpp_redis::reply r = sel.get();
                if (r.ko()) {
//                    log_error("Redis [host={0}][port={1}] select db_index={2} failed, error: {3}", host_, port_,
//                              db_index_, r.as_string());
                    std::ostringstream oss;
                    oss << "Redis [host={" << host_ << "}][port=" << port_ << "] select db_index=" << db_ << " failed, error: " << r.as_string();
                    MY_THROW(base::BaseException, oss.str());
                }
            }
            return client;
        }

        inline cpp_redis::client *RedisPool::find_mru() {
            PoolIt mru = std::max_element(pool_.begin(), pool_.end());
            if (mru != pool_.end() && !mru->in_use_) {
                mru->in_use_ = true;
                return mru->client_;
            } else {
                return nullptr;
            }
        }

        inline void RedisPool::release(cpp_redis::client *pc) {
            std::lock_guard<std::mutex> lock(mutex_);    // ensure we're not interfered with
            pc->commit();

            for (PoolIt it = pool_.begin(); it != pool_.end(); ++it) {
                if (it->client_ == pc) {
                    it->in_use_ = false;
                    it->last_used_ = time(0);
//                    log_trace("find and release redis client");
                    break;
                }
            }
//            log_trace("release cpp_redis::client");
        }

        inline cpp_redis::client *RedisPool::raw_grab() {
//            std::lock_guard<std::mutex> lock(mutex_);    // ensure we're not interfered with
            remove_old_connections();
            if (cpp_redis::client *mru = find_mru()) {
                return mru;
            } else {
                // No free connections, so create and return a new one.
                pool_.push_back(RedisInfo(create()));
                return pool_.back().client_;
            }
        }

        inline void RedisPool::clear(bool all) {
            std::lock_guard<std::mutex> lock(mutex_);    // ensure we're not interfered with

            PoolIt it = pool_.begin();
            while (it != pool_.end()) {
                if (all || !it->in_use_) {
                    remove(it++);
                } else {
                    ++it;
                }
            }
        }

        inline void RedisPool::remove(const cpp_redis::client *pc) {
            std::lock_guard<std::mutex> lock(mutex_);    // ensure we're not interfered with

            for (PoolIt it = pool_.begin(); it != pool_.end(); ++it) {
                if (it->client_ == pc) {
                    remove(it);
                    return;
                }
            }
        }

        inline void RedisPool::remove(const PoolIt &it) {
            // Don't grab the mutex.  Only called from other functions that do
            // grab it.
            destroy(it->client_);
            pool_.erase(it);
        }

        inline void RedisPool::destroy(cpp_redis::client *pc) {
            delete pc;
            log_trace("destroy cpp_redis::client");
        }

//// remove_old_connections ////////////////////////////////////////////
// Remove connections that were last used too long ago.

        inline void RedisPool::remove_old_connections() {
            TooOld<RedisInfo> too_old(max_idle_time());

            PoolIt it = pool_.begin();
            while ((it = std::find_if(it, pool_.end(), too_old)) != pool_.end()) {
                remove(it++);
            }
        }


//// safe_grab /////////////////////////////////////////////////////////

        inline bool RedisPool::ping(cpp_redis::client *c) {
            return c->is_connected();
        }

        inline std::shared_ptr<cpp_redis::client> RedisPool::safe_grab() {
            cpp_redis::client *pc;
            std::lock_guard<std::mutex> lock(mutex_);    // ensure we're not interfered with
            while (!ping(pc = raw_grab())) {
                remove(pc);
//                pc = nullptr;
            }
            std::shared_ptr<cpp_redis::client> ptr(pc, [&](cpp_redis::client *c) { release(c); });
            return ptr;
        }


    } /* redis */
} /* base */
#endif /* end of include guard: _BASE_REDIS_REDIS_POOL_IMPL_HPP_ */
