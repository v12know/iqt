#include_directories(${PROJECT_SOURCE_DIR}/extra)
#include_directories(${PROJECT_SOURCE_DIR}/include)
#include_directories(${PROJECT_SOURCE_DIR}/src)
SET(LIB_NAME iqt)

#aux_source_directory(base BASE_LIST)
#aux_source_directory(ctp CTP_LIST)
#aux_source_directory(stgy STGY_LIST)
#aux_source_directory(GLOB_RECURSE . SRC_LIST)
#file(GLOB_RECURSE SRC_LIST "*.cpp")

SET(SRC_LIST
        base/lock/rwlock.hpp
        base/lock/rwlock_impl.hpp
        base/lock/spin_mutex.hpp

        base/milo/dtoa_milo.h

        base/portal/constructor_stats.hpp
        base/portal/portal_init.hpp
        base/portal/portal_init_impl.hpp

        base/redis/redis_pool_impl.hpp
        base/redis/redis_pool.hpp
        base/redis/redis_factory.cpp
        base/redis/redis_factory.hpp

        base/enum_wrap.hpp
        base/exception.h
        base/exception.cpp
        base/log.h
        base/log.cpp
        base/macro.hpp
#        base/ppk_assert.h_bak
#        base/ppk_assert.cpp_bak
        base/raii.hpp
        base/sig_catch.hpp
        base/time_span.hpp
        base/timer1.hpp
        base/timer.cpp
        base/timer.hpp
        base/util.hpp
        base/util_impl.hpp

        dao/base_dao.hpp
        dao/future_info_dao.hpp
        dao/stgy_dao.hpp
        dao/rpt_dao.hpp
        dao/txn_dao.hpp
        dao/group_dao.hpp
        dao/order_dao.hpp

        event/event.h
        event/event.cpp
        event/event_type.h
        event/event_source.h
        event/event_source.cpp

        model/account.h
        model/bar.h
        model/future_info.h
        model/group.h
        model/rpt.h
        model/instrument.h
        model/order.h
        model/position.h
        model/stgy.h
        model/tick.cpp
        model/tick.h
        model/trade.h
        model/txn.h

        trade/observer/md_subject.cpp
        trade/observer/md_subject.h
        trade/observer/stgy_observer.cpp
        trade/observer/stgy_observer.h

        risk/abs_validator.h

        service/future_info_service.hpp
        service/stgy_service.hpp
        service/rpt_service.hpp
        service/txn_service.hpp
        service/group_service.hpp
        service/order_service.hpp

        trade/abs_broker.h
        trade/api_future.cpp
        trade/api_future.h
        trade/future_account.cpp
        trade/future_account.h
        trade/future_position.cpp
        trade/future_position.h
        trade/portfolio.cpp
        trade/portfolio.h
        trade/qry_queue.h
        trade/qry_queue.cpp

        const.hpp

        env.cpp
        env.h

        global.h
        global.cpp
#        launch.h
#        launch.cpp
        #main.cpp
        )


message("SRC_LIST=${SRC_LIST}")

#find_library(CTP_LIBRARY NAMES ctp PATHS ${PROJECT_SOURCE_DIR}/lib)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(DEPS_LIBRARIES ${PROJECT_SOURCE_DIR}/lib)

add_library(${LIB_NAME} SHARED ${SRC_LIST})
#	target_link_libraries(${LIB_NAME} PRIVATE pybind11::module)
TARGET_LINK_LIBRARIES(${LIB_NAME} pthread cpp_redis)

add_subdirectory(ctp)
add_subdirectory(stgy)
add_subdirectory(portal)

#[[if (VTEST)
    message("VTEST defined")
    add_definitions(-DVTEST)
    #add_definitions(-DVTEST)
    add_subdirectory(test)
else (VTEST)
    if (NOT VBIN)
        add_subdirectory(portal)
    endif (NOT VBIN)
endif (VTEST)]]
