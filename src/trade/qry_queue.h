#ifndef _IQT_TRADE_QRY_QUEUE_H_
#define _IQT_TRADE_QRY_QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <functional>
#include <chrono>
#include <queue>
#include <thread>
#include <memory>
#include <atomic>
#include <sstream>

#include "base/macro.hpp"
#include "base/exception.h"
#include "global.h"
//#include "base/log.h"

namespace iqt {
    namespace trade {
// Forward declarations.

        struct QryItem {
            int req_id = 0;
            int ret_id = 0;
            int error_id = 0;
            std::string error_msg;
            std::function<int(int)> qry_func;
            std::shared_ptr<void> rsp_data = nullptr;
            std::condition_variable rsp_cond;
            std::mutex mutex;
        };

        class QryQueue {
        public:
            QryQueue() = default;

            void start();

            template <typename DATA>
            std::shared_ptr<DATA> qry(std::function<int(int)> qry_func);

            template <typename DATA>
            void on_rsp(int request_id, std::shared_ptr<DATA> rsp_data);

            void on_err(int request_id, int error_id, const std::string &error_msg);

            int next_req_id() { return ++request_id_; }

        protected:
            void run();

            void consume();
        private:
            static const size_t MAX_BUFFER_SIZE = 1000;
            std::queue<std::shared_ptr<QryItem>> queue_;
            std::chrono::system_clock::time_point last_;
            std::condition_variable not_empty_cond_, not_full_cond_;
            std::mutex mutex_;
            std::thread thread_;
            bool run_flag_ = true;
            std::atomic_int request_id_{0};
            int qry_num_per_sec_ = 1;

            static std::chrono::seconds sec;
            static std::chrono::seconds timeout_sec;
        };

        template <typename DATA>
        inline std::shared_ptr<DATA> QryQueue::qry(std::function<int(int)> qry_func) {
            std::unique_lock<std::mutex> lock(mutex_);
            while (queue_.size() == MAX_BUFFER_SIZE) {
                not_full_cond_.wait(lock);
            }
            auto qry_item = std::make_shared<QryItem>();
            qry_item->qry_func = qry_func;
            queue_.push(qry_item);
            std::unique_lock<std::mutex> qry_lock(qry_item->mutex);
            lock.unlock();
            not_empty_cond_.notify_all();

            qry_item->rsp_cond.wait_for(qry_lock, timeout_sec);
            qry_lock.unlock();
            if (qry_item->ret_id != 0) {
                std::ostringstream oss;
                oss << "[" << qry_item->ret_id << "]" << global::mapRetVal(qry_item->ret_id);
                MY_THROW(base::ReqException, oss.str());
            }
            if (qry_item->error_id != 0) {
                std::ostringstream oss;
                oss << "[" << qry_item->error_id << "]" << qry_item->error_msg;
                MY_THROW(base::RspException, oss.str());
            }
//            log_info("hehehe000000000000000-------------------------");
            return std::static_pointer_cast<DATA>(qry_item->rsp_data);
        }

        template <typename DATA>
        inline void QryQueue::on_rsp(int request_id, std::shared_ptr<DATA> rsp_data) {
            auto &cur_qry_item = queue_.front();
            std::unique_lock<std::mutex> qry_lock(cur_qry_item->mutex);
            if (cur_qry_item->req_id == request_id) {
                cur_qry_item->rsp_data = rsp_data;
                qry_lock.unlock();
                cur_qry_item->rsp_cond.notify_all();
            }
//            log_info("hehehe111111111================================");
        }
    } /* trade */
} /* iqt */
#endif /* end of include guard: _IQT_TRADE_QRY_QUEUE_H_ */
