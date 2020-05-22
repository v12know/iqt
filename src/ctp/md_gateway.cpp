#include "ctp/md_gateway.h"

#include <regex>
#include <thread>
#include <event/event.h>

#include "base/log.h"
#include "base/exception.h"
#include "ctp/md_api.h"
#include "model/tick.h"
#include "env.h"

namespace iqt {
    namespace trade {

        CtpMdGateway::CtpMdGateway(int retry_times, int retry_interval) :
                md_subject_(trade::observer::MdSubject::instance()), retry_times_(retry_times),
                retry_interval_(retry_interval) {}

        CtpMdGateway::~CtpMdGateway() {
            if (md_api_) {
                delete (md_api_);
            }
        }


        void
        CtpMdGateway::connect(const std::string &broker_id, const std::string &user_id, const std::string &password,
                           const std::string &address) {
            if (md_api_) {
                delete md_api_;
            }
            md_api_ = new trade::CtpMdApi(this, broker_id, user_id, password, address);

            bool success = false;
            for (auto i = 0; i < retry_times_; ++i) {
                md_api_->connect();
                wait_for();
                if (md_api_->logged_in_) {
                    log_warn("CTP 行情服务器登录成功");
                    success = true;
                    break;
                }
            }
            if (!success) {
                MY_THROW(base::BaseException, "CTP 行情服务器连接或登录超时");
            }
            auto lambda_universe = [&](std::vector<std::string> &universe) {
                std::vector<std::string> tmp_vec;
                for (auto &order_book_id : universe) {
                    if (Env::instance()->universe.count(order_book_id) == 0 && Env::instance()->instruments) {
                        tmp_vec.push_back((*Env::instance()->instruments)[order_book_id]->instrument_id);
                    }
                }
                if (tmp_vec.size() > 0) {
                    std::vector<std::shared_ptr<std::vector<std::string>>> tmp_vec_vec;
                    std::shared_ptr<std::vector<std::string>> cur_vec;
                    for (decltype(tmp_vec.size()) i = 0; i < tmp_vec.size(); ++i) {
                        if (i % 100 == 0) {
                            cur_vec = std::make_shared<std::vector<std::string>>();
                            tmp_vec_vec.emplace_back(cur_vec);
                        }
                        cur_vec->emplace_back(tmp_vec[i]);
                    }
                    for (decltype(tmp_vec_vec.size()) k = 0; k < tmp_vec_vec.size(); ++k) {
                        auto vec = tmp_vec_vec[k];
                        std::string order_book_ids;
                        for (decltype(vec->size()) j = 0; j < vec->size(); ++j) {
                            auto &order_book_id = (*vec)[j];
                            if (j > 0) order_book_ids.append(", ");
                            order_book_ids.append(order_book_id);
                        }
                        log_trace("POST_UNIVERSE_CHANGED [{0}]", order_book_ids);
                        Env::instance()->universe.insert(vec->begin(), vec->end());
                        subscribe(*vec);
                        if (k < tmp_vec_vec.size() - 1) sleep(1);
                    }
                }
            };
            event::EventBus::add_listener<std::vector<std::string> &>(event::POST_UNIVERSE_CHANGED,
                                                                    {lambda_universe, event::ALL_TARGET_ID});
        }

        void CtpMdGateway::join() {
            md_api_->join();
        }

        void CtpMdGateway::close() {
            if (md_api_) {
                md_api_->close();
                delete md_api_;
                md_api_ = nullptr;
            }
        }

        void CtpMdGateway::subscribe() {
            subscribe(std::vector<std::string>(Env::instance()->universe.begin(), Env::instance()->universe.end()));
        }

        void CtpMdGateway::subscribe(const std::vector<std::string> &subscribed) {
            if (subscribed.size() == 0)
                return;
            md_api_->subscribe(subscribed);
//            md_api_->subscribe({"v1801"});
        }


        void CtpMdGateway::on_tick(std::shared_ptr<model::Tick> tick) {
//            snapshot_[tick->order_book_id] = tick;
//            std::cout << "tick: " << tick->to_string() << std::endl;
//            Env::instance()->snapshot.set_tick(tick);
            Env::instance()->snapshot[tick->order_book_id] = tick;
//            log_debug("new tick: order_book_id={}", tick->order_book_id);
            md_subject_->notify(tick);
        }


        void CtpMdGateway::wait_for(int timeout) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (timeout == 0) {
                connect_cond_.wait(lock);
            } else {
                std::chrono::seconds sec(timeout);
                connect_cond_.wait_for(lock, sec);
            }
        }

        void CtpMdGateway::notify() {
            connect_cond_.notify_all();
        }

    } /* ctp */

} /* iqt */ 

