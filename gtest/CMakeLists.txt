#include_directories(${PROJECT_SOURCE_DIR}/extra)
#include_directories(${PROJECT_SOURCE_DIR}/include)
#include_directories(${PROJECT_SOURCE_DIR}/src)


#SET(PYBIND11_PYTHON_VERSION "3.5")
SET(BIN_NAME gtest_bin)


SET(SRC_LIST
#        base/test_timer.cpp
#        service/test_future_info_service.cpp
        test_env.cpp
        test_env.h
        test_main.cpp
        test_misc.cpp
        )

message("SRC_LIST=${SRC_LIST}")

#find_library(CTPAPI_MD_LIBRARY NAMES thostmduserapi PATHS ${PROJECT_SOURCE_DIR}/lib)
#find_library(CTPAPI_TD_LIBRARY NAMES thosttraderapi PATHS ${PROJECT_SOURCE_DIR}/lib)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#pybind11_add_module(${LIB_NAME} SHARED ${SRC_LIST})
add_executable(${BIN_NAME} ${SRC_LIST})
#target_link_libraries(${LIB_NAME} PRIVATE pybind11::module)
TARGET_LINK_LIBRARIES(${BIN_NAME} iqt gtest cpp_redis)
