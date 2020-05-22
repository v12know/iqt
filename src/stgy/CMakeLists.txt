#include_directories(${PROJECT_SOURCE_DIR}/extra)
#include_directories(${PROJECT_SOURCE_DIR}/include)
#include_directories(${PROJECT_SOURCE_DIR}/src)


SET(LIB_NAME portal_stgy)


SET(SRC_LIST
        portal/portal_stgy.cpp
        portal/portal_stgy.h

        single_grid/single_grid_stgy.cpp
        single_grid/single_grid_stgy.h
        single_grid/single_grid_stgy_helper.hpp

        time_line/time_line_stgy.cpp
        time_line/time_line_stgy.h
        time_line/time_line_stgy_helper.hpp

        abs_stgy.h

        stgy_context.h
        stgy_executor.cpp
        stgy_executor.h

)

message("SRC_LIST=${SRC_LIST}")

message(STATUS "CTPAPI_TD_LIBRARY=${CTPAPI_TD_LIBRARY}")
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/pyiqt/stgy)

#pybind11_add_module(${LIB_NAME} SHARED ${SRC_LIST})
add_library(${LIB_NAME} SHARED ${SRC_LIST})
target_link_libraries(${LIB_NAME} PRIVATE pybind11::module iqt ctp pthread)
set_target_properties(${LIB_NAME} PROPERTIES PREFIX "")

