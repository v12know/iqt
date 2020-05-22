#ifndef _IQT_CTP_TRADE_GATEWAY_H_
#define _IQT_CTP_TRADE_GATEWAY_H_

#include <string>
#include <unordered_map>
#include <list>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <utility>



namespace base {
    namespace lock {
        class SpinMutex;
    }
}

namespace iqt {
    namespace trade {
        class FutureAccount;
        class Positions;
        struct Portfolio;

        enum class POSITION_EFFECT_TYPE;
    }
    namespace model {
        struct Instrument;
        struct Commission;
        struct FutureInfo;
        struct Account;
        struct Position;
        struct Order;
        struct Trade;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Commission>> CommissionMap;
        typedef std::shared_ptr<model::CommissionMap> CommissionMapPtr;

        typedef std::shared_ptr<model::Instrument> InstrumentPtr;
        typedef std::unordered_map<std::string, InstrumentPtr> InstrumentMap;
        typedef std::shared_ptr<model::InstrumentMap> InstrumentMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::FutureInfo>> FutureInfoMap;
        typedef std::shared_ptr<model::FutureInfoMap> FutureInfoMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Position>> PositionMap;
        typedef std::shared_ptr<model::PositionMap> PositionMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Order>> OrderMap;
        typedef std::shared_ptr<model::OrderMap> OrderMapPtr;

        typedef std::unordered_map<std::string, std::list<std::shared_ptr<model::Trade>>> TradeListMap;
        typedef std::shared_ptr<model::TradeListMap> TradeListMapPtr;
    }
    namespace trade {

        class CtpTradeApi;
        class CtpMdGateway;

        class DataCache {
            friend class CtpTradeGateway;
        public:
            DataCache();

            void cache_ins(model::InstrumentMapPtr ins_cache);

            void cache_commission(const std::string &order_book_id, std::shared_ptr<model::Commission> commission);

            void cache_position(model::PositionMapPtr position_cache);

            void cache_account(std::shared_ptr<model::Account> account);

            void cache_trade(std::shared_ptr<model::Trade> trade);

            void cache_open_order(std::shared_ptr<model::Order> order);

            void remove_open_order(std::shared_ptr<model::Order> order);

            std::shared_ptr<model::Order> get_cached_order(std::shared_ptr<model::Order> order);

            std::shared_ptr<model::Order> get_cached_order(std::shared_ptr<model::Trade> trade);

            void cache_order(std::shared_ptr<model::Order> order);

            void cache_sys_order(std::shared_ptr<model::Order> order);

            std::shared_ptr<trade::Positions> positions();

            std::pair<std::shared_ptr<trade::FutureAccount>, double>  account();

        private:
            model::InstrumentMapPtr ins_;
            model::PositionMapPtr pos_;
            model::TradeListMapPtr trades_;
            model::OrderMapPtr orders_;
            model::OrderMapPtr sys_orders_;
            std::list<std::shared_ptr<model::Order>> open_orders_;
            std::shared_ptr<model::Account> account_;
            model::FutureInfoMapPtr future_info_;
        };

        class CtpTradeGateway final {
        public:
            CtpTradeGateway(int retry_times = 5, int retry_interval = 1);

            ~CtpTradeGateway();

            void init_env();

            void connect(const std::string &broker_id, const std::string &user_id, const std::string &password,
                         const std::string &address, const std::string &pub_resume_type,
                         const std::string &priv_resume_type);

            std::string get_account_id() const {
                return account_id_;
            }

            void join();

            void close();

            std::list<std::shared_ptr<model::Order>> &open_orders() {
                return cache_->open_orders_;
            }

            std::shared_ptr<trade::Portfolio> get_portfolio();

            void submit_order(std::shared_ptr<model::Order> order);

            void cancel_order(std::shared_ptr<model::Order> order);

            void on_order(std::shared_ptr<model::Order> ord);

            double calc_commission(std::shared_ptr<model::Trade> trade, const trade::POSITION_EFFECT_TYPE &poisition_effect);

            void on_trade(std::shared_ptr<model::Trade> trade);

            void wait_for(int timeout = 0);

            void notify();

        private:
            std::string get_trading_date();
            void confirm_settlement_info();
            void qry_instrument();
            void qry_account();
            void qry_position();
            void qry_order();
            model::CommissionMapPtr qry_commission(const std::string &order_book_id);
            void qry_commission();

            DataCache *cache_;
            trade::CtpTradeApi *td_api_ = nullptr;
            int retry_times_;
            int retry_interval_;
            std::string data_update_date_ = "19700101";
            std::string account_id_;
            std::mutex mutex_;
            std::condition_variable connect_cond_;
//            std::string collect_pattern_;
            base::lock::SpinMutex *sm_;
        };

    } /* ctp */
}/* iqt */
#endif /* end of include guard: _IQT_CTP_TRADE_GATEWAY_H_ */
