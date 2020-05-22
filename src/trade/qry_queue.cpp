#include "trade/qry_queue.h"

//#include "base/log.h"

namespace iqt {
    namespace trade {

        std::chrono::seconds QryQueue::sec(1);
        std::chrono::seconds QryQueue::timeout_sec(5);
        void QryQueue::start() {
            thread_ = std::thread(std::mem_fn(&QryQueue::run), this);
            thread_.detach();
        }

        void QryQueue::run() {
            last_ = std::chrono::system_clock::now() - sec;
            while (run_flag_) {
                consume();
            }
        }


        void QryQueue::consume() {
            auto interval = sec / qry_num_per_sec_;
            std::unique_lock<std::mutex> lock(mutex_);
            for (int i=0; i<qry_num_per_sec_; ++i) {
                while (queue_.size() == 0) {
                    not_empty_cond_.wait(lock);
                }
                auto &cur_qry_item = queue_.front();
                std::unique_lock<std::mutex> qry_lock(cur_qry_item->mutex);
                cur_qry_item->req_id = next_req_id();
                cur_qry_item->ret_id = cur_qry_item->qry_func(cur_qry_item->req_id);
                if (cur_qry_item->ret_id == 0) {
                    cur_qry_item->rsp_cond.wait_for(qry_lock, timeout_sec);
                    qry_lock.unlock();
                } else {
                    qry_lock.unlock();
                    cur_qry_item->rsp_cond.notify_all();
                }
                queue_.pop();

                while (last_ + interval > std::chrono::system_clock::now()) {
                    not_empty_cond_.wait_until(lock, last_ + interval);
                }
                last_ = std::chrono::system_clock::now();
            }
            lock.unlock();
            not_full_cond_.notify_all();
        }



        void QryQueue::on_err(int request_id, int error_id, const std::string &error_msg) {
            auto &cur_qry_item = queue_.front();
            std::unique_lock<std::mutex> qry_lock(cur_qry_item->mutex);
            if (cur_qry_item->req_id == request_id) {
                cur_qry_item->error_id = error_id;
                cur_qry_item->error_msg = error_msg;
                qry_lock.unlock();
                cur_qry_item->rsp_cond.notify_all();
            }
        }

    } /* trade */

} /* iqt */

/*int main() {
    using namespace iqt::trade;
    QryQueue q;
    q.start();
    q.qry<int>([]() {return 1;});
    return 0;
}*/
