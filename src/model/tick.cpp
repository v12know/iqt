#include "tick.h"


#include "env.h"

namespace iqt {

namespace model {

//std::atomic_int gRequestId(0);


        void Tick::fix_settlement() {
            settlement = volume ? total_turnover / volume : 0;
            auto ins = Env::instance()->get_instrument(order_book_id);
            if (ins->exchange_id != trade::EXCHANGE_ID::CZCE) {
                settlement /= ins->contract_multiplier;
            }
        }

        void Tick::fix_tick() {
            settlement = volume ? total_turnover / volume : 0;
            auto ins = Env::instance()->get_instrument(order_book_id);
            if (ins->exchange_id != trade::EXCHANGE_ID::CZCE) {
                settlement /= ins->contract_multiplier;
            } else {
                static std::chrono::milliseconds ms500(500);
                static std::unordered_map<std::string, std::shared_ptr<model::Tick>> tick_map;
                // for czce ms bug
                if (tick_map.count(order_book_id) > 0) {
                    auto t = tick_map[order_book_id];
                    if (t->datetime == datetime) {//when the same second, millisecond field is the same
                        datetime += ms500;//just plus 500ms to fix it
                    }
                    t->datetime = datetime;
                } else {
                    auto t = std::make_shared<model::Tick>();
                    t->order_book_id = order_book_id;
                    t->datetime = datetime;
                    tick_map[order_book_id] = t;
                }
            }
        }
} /* global */
} /* iqt */ 
