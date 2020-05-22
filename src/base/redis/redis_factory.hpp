#ifndef _BASE_REDIS_REDIS_POOL_FACTORY_HPP_
#define _BASE_REDIS_REDIS_POOL_FACTORY_HPP_

#include <mutex>
#include <string>
#include <unordered_map>
#include <memory>

#include <cpp_redis/cpp_redis>

#include "base/macro.hpp"


namespace base {
//    namespace cpp_redis {
//        class client;
//    }

    namespace redis {
        class RedisPool;

        class RedisFactory {
        public:
            static RedisFactory *instance();

            ~RedisFactory() {
                destruct();
            }

            void destruct();

            static void create_redis(const std::string &host, int port, int db, const std::string &password);

            static std::shared_ptr<cpp_redis::client> get_redis();

            static void create_redis_by_id(const std::string &host, int port, int db, const std::string &password, const std::string &redis_id);

            static std::shared_ptr<cpp_redis::client> get_redis_by_id(const std::string &redis_id);

        private:
            DISALLOW_COPY_AND_ASSIGN(RedisFactory);

            RedisFactory() = default;

            /* data */
            std::unordered_map<std::string, RedisPool *> pool_map_;
            RedisPool *def_pool_;
        };

    } /* redis */
}/* base */

#endif /* end of include guard: _BASE_REDIS_REDIS_POOL_FACTORY_HPP_ */
