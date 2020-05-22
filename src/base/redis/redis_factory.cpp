#include "redis_factory.hpp"

#include "base/log.h"
#include "redis_pool.hpp"

namespace base {
    namespace redis {

        RedisFactory *RedisFactory::instance() {
            static RedisFactory s_RedisFactory;
            return &s_RedisFactory;
        }

        void RedisFactory::destruct() {
            for (auto &i : pool_map_) {
                delete i.second;
            }
            if (def_pool_) {
                delete def_pool_;
            }
        }

        void RedisFactory::create_redis(const std::string &host, int port, int db, const std::string &password) {
            instance()->def_pool_ = new RedisPool(host, port, db, password);
        }

        std::shared_ptr<cpp_redis::client> RedisFactory::get_redis() {
            return instance()->def_pool_->grab();
        }

        void RedisFactory::create_redis_by_id(const std::string &host, int port, int db, const std::string &password, const std::string &redis_id) {
            instance()->pool_map_[redis_id] = new RedisPool(host, port, db, password);
        }

        std::shared_ptr<cpp_redis::client> RedisFactory::get_redis_by_id(const std::string &redis_id) {
            return instance()->pool_map_.at(redis_id)->grab();
        }
    } /* redis */
} /* base */
