#ifndef _IQT_DAO_BASE_DAO_HPP_
#define _IQT_DAO_BASE_DAO_HPP_

#include <sstream>

//#include "model/rpt.h"
#include "base/redis.h"


namespace iqt {
    namespace dao {
        using namespace iqt::model;

        class BaseDao {
        public:
            int64_t exists(cpp_redis::client &r, const std::vector<std::string> &keys) {
                auto exists = r.exists(keys);
                r.sync_commit();
                return exists.get().as_integer();
            }

            int64_t hexists(cpp_redis::client &r, const std::string& key, const std::string& field) {
                auto exists = r.hexists(key, field);
                r.sync_commit();
                return exists.get().as_integer();
            }

            bool
            sismember(cpp_redis::client &r, const std::string &key, const std::string &member) {
                auto sismember = r.sismember(key, member);
                r.sync_commit();
                return sismember.get().as_integer();
            }

            void sadd(cpp_redis::client &r, const std::string &key,
                             const std::vector<std::string> &members) {
                r.sadd(key, members);
            }
            void zadd(cpp_redis::client &r, const std::string& key, const std::vector<std::string>& options, const std::multimap<std::string, std::string>& score_members) {
                r.zadd(key, options, score_members);
            }

            int64_t incr(cpp_redis::client &r, const std::string &key) {
                auto incr = r.incr(key);
                r.sync_commit();
                return incr.get().as_integer();
            }

/*            std::string incr_str(cpp_redis::client &r, const std::string &key) {
                auto incr = r->incr(key);
                r->sync_commit();
                return incr.get().as_string();
            }*/

            void set(cpp_redis::client &r, const std::string &key, const std::string &value) {
                r.set(key, value);
            }
            void expire(cpp_redis::client &r, const std::string &key, int seconds) {
                r.expire(key, seconds);
            }
            void expireat(cpp_redis::client &r, const std::string &key, int timestamp) {
                r.expireat(key, timestamp);
            }

            void hset(cpp_redis::client &r, const std::string &key, const std::string &field, const std::string &value) {
                r.hset(key, field, value);
            }

            void hdel(cpp_redis::client &r, const std::string &key, const std::vector<std::string>& fields) {
                r.hdel(key, fields);
            }

            std::string get(cpp_redis::client &r, const std::string &key) {
                auto get = r.get(key);
                r.sync_commit();
                return get.get().as_string();
            }

            std::string hget(cpp_redis::client &r, const std::string &key, const std::string &field) {
                auto get = r.hget(key, field);
                r.sync_commit();
                return get.get().as_string();
            }

            void del(cpp_redis::client &r, const std::vector<std::string>& key) {
                r.del(key);
            }
        };
    } /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_BASE_DAO_HPP_ */
