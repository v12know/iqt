#ifndef _IQT_STGY_ABS_STGY_H_
#define _IQT_STGY_ABS_STGY_H_

#include <pybind11/pybind11.h>

#include "model/tick.h"
#include "stgy/stgy_context.h"

namespace iqt {

    namespace stgy {
        namespace py = pybind11;

        class AbstractStgy {
        public:
            AbstractStgy() = default;

            virtual ~AbstractStgy() = default;

            void set_stgy_context(StgyContext *context) { context_ = context; }
            StgyContext *get_stgy_context() const { return context_; }

            virtual bool init(StgyContext *context) { return true; }

            virtual void handle_tick(StgyContext *context, std::shared_ptr<model::Tick> tick) = 0;

            virtual void before_trading(StgyContext *context) {}

            virtual void after_trading(StgyContext *context) {}

        private:
            StgyContext *context_;
        };

        class PyAbstractStgy : public AbstractStgy {
        public:
            using AbstractStgy::AbstractStgy; // Inherit constructors
            bool init(StgyContext *context) override {
                PYBIND11_OVERLOAD(bool, AbstractStgy, init, context);
            }
            void handle_tick(StgyContext *context, std::shared_ptr<model::Tick> tick) override {
                PYBIND11_OVERLOAD_PURE(void, AbstractStgy, handle_tick, context, tick);
            };
            void before_trading(StgyContext *context) override {
                PYBIND11_OVERLOAD(void, AbstractStgy, before_trading, context);
            };
            void after_trading(StgyContext *context) override {
                PYBIND11_OVERLOAD(void, AbstractStgy, after_trading, context);
            };
        };
    } /* stgy  */
} /* iqt */
#endif /* end of include guard: _IQT_STGY_ABS_STGY_H_ */
