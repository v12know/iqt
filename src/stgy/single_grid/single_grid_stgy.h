#ifndef _IQT_STGY_SINGLE_GRID_SINGLE_GRID_STGY_H_
#define _IQT_STGY_SINGLE_GRID_SINGLE_GRID_STGY_H_

#include <tuple>
#include "stgy/abs_stgy.h"

namespace cpp_redis {
    class client;
}
namespace iqt {
    namespace trade {
        class ApiFuture;
    }

    namespace model {
        struct Txn;
        struct Slot;
        struct TxnCost;
        struct OpenGroup;
        struct CloseGroup;

        typedef std::shared_ptr<Txn> TxnPtr;
        typedef std::shared_ptr<Slot> SlotPtr;
        typedef std::shared_ptr<TxnCost> TxnCostPtr;
        typedef std::shared_ptr<OpenGroup> OpenGroupPtr;
        typedef std::shared_ptr<CloseGroup> CloseGroupPtr;


        typedef std::tuple<TradePtr, OrderPtr> TradeTuple;
    }


    namespace stgy {
        class SingleGridConfig;
        class SingleGridStgyHelper;
        using namespace model;



        class SingleGridStgy : public AbstractStgy {
        public:
            explicit SingleGridStgy(const SingleGridConfig *conf);

            ~SingleGridStgy();


            bool init(StgyContext *context) override;

            void handle_tick(StgyContext *context, std::shared_ptr<Tick> tick) override;

            void before_trading(StgyContext *context) override;

            void after_trading(StgyContext *context) override;

            void post_after_trading(StgyContext *context);


            void update_signal(double ma5_ols, double ma20_ols, double ma20_dis, double atr);
        private:
            void handle_txn_status(cpp_redis::client &r, std::shared_ptr<Tick> tick, int sign, Txn &txn);

            void on_order(std::shared_ptr<Order> order);

            void on_trade(TradeTuple &trade);

            void post_open(cpp_redis::client &r, int sign, OpenGroupPtr open_group);
            void post_lock(cpp_redis::client &r, int sign, OpenGroupPtr open_group);

            void post_buy_open(cpp_redis::client &r, std::shared_ptr<Order> ord);
            void post_sell_open(cpp_redis::client &r, std::shared_ptr<Order> ord);

            void post_close(cpp_redis::client &r, int sign, CloseGroupPtr close_group);
            void post_unlock(cpp_redis::client &r, int sign, CloseGroupPtr close_group);

            void post_sell_close(cpp_redis::client &r, std::shared_ptr<Order> ord);
            void post_buy_close(cpp_redis::client &r, std::shared_ptr<Order> ord);

            void post_open_after_trade(cpp_redis::client &r, int sign, OpenGroupPtr open_group);
            void post_lock_after_trade(cpp_redis::client &r, int sign, OpenGroupPtr open_group);

            void post_buy_open_after_trade(cpp_redis::client &r, TradeTuple &trade);
            void post_sell_open_after_trade(cpp_redis::client &r, TradeTuple &trade);

            void post_close_after_trade(cpp_redis::client &r, int sign, CloseGroupPtr close_group);
            void post_unlock_after_trade(cpp_redis::client &r, int sign, CloseGroupPtr close_group);

            void post_sell_close_after_trade(cpp_redis::client &r, TradeTuple &trade);
            void post_buy_close_after_trade(cpp_redis::client &r, TradeTuple &trade);

            typedef std::function<std::vector<std::shared_ptr<Order>>(const std::string &order_book_id, int amount, double price)> order_func_t;
            void lock(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func);
            void open(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func);

            void buy_open(cpp_redis::client &r, std::shared_ptr<Tick> tick);
            void sell_open(cpp_redis::client &r, std::shared_ptr<Tick> tick);

            void unlock(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func);
            void close(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func);

            void sell_close(cpp_redis::client &r, std::shared_ptr<Tick> tick);
            void buy_close(cpp_redis::client &r, std::shared_ptr<Tick> tick);

        private:
            trade::ApiFuture *api_future_;
            SingleGridStgyHelper *helper_;

//            std::string stgy_id_;
            const SingleGridConfig *config_;
        };

    } /* stgy  */
} /* iqt */
#endif /* end of include guard: _IQT_STGY_SINGLE_GRID_SINGLE_GRID_STGY_H_ */
