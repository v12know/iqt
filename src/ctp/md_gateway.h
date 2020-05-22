#ifndef _IQT_CTP_MD_GATEWAY_H_
#define _IQT_CTP_MD_GATEWAY_H_

#include <string>
#include <unordered_map>
#include <memory>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <condition_variable>


namespace iqt {
    namespace model {
        struct Tick;
    }
    namespace trade {
        namespace observer {
            class MdSubject;
        } /* observer */
    } /* trade */
    namespace trade {

        class CtpMdApi;

        class CtpMdGateway final {
            friend class CtpMdApi;

        public:
            CtpMdGateway(int retry_times = 5, int retry_interval = 1);

            ~CtpMdGateway();

            void join();

            void close();

            void subscribe();

            void subscribe(const std::vector<std::string> &subscribed);

            void connect(const std::string &broker_id, const std::string &user_id, const std::string &password,
                         const std::string &address);

            std::shared_ptr<model::Tick> snapshot(const std::string &order_book_id) {
                return snapshot_[order_book_id];
            }

            void on_tick(std::shared_ptr<model::Tick> tick);

            void wait_for(int timeout = 0);

            void notify();

        private:
            trade::CtpMdApi *md_api_ = nullptr;
            trade::observer::MdSubject *md_subject_;
            std::unordered_map<std::string, std::shared_ptr<model::Tick>> snapshot_;
//            std::unordered_set<std::string> subscribed_set_;
            int retry_times_;
            int retry_interval_;
            std::mutex mutex_;
            std::condition_variable connect_cond_;
        };

    } /* ctp */
}/* iqt */
#endif /* end of include guard: _IQT_CTP_MD_GATEWAY_H_ */
