#include_directories(${PROJECT_SOURCE_DIR}/extra)
#include_directories(${PROJECT_SOURCE_DIR}/include)
#include_directories(${PROJECT_SOURCE_DIR}/src)


#SET(PYBIND11_PYTHON_VERSION "3.5")
SET(LIB_NAME ctp)


SET(SRC_LIST
        ctp_broker.h
        md_api.cpp
        md_api.h
        md_gateway.cpp
        md_gateway.h
        trade_api.cpp
        trade_api.h
        trade_gateway.cpp
        trade_gateway.h
        )

message("SRC_LIST=${SRC_LIST}")

find_library(CTPAPI_MD_LIBRARY NAMES thostmduserapi PATHS ${PROJECT_SOURCE_DIR}/lib)
find_library(CTPAPI_TD_LIBRARY NAMES thosttraderapi PATHS ${PROJECT_SOURCE_DIR}/lib)
message(STATUS "CTPAPI_TD_LIBRARY=${CTPAPI_TD_LIBRARY}")
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#pybind11_add_module(${LIB_NAME} SHARED ${SRC_LIST})
add_library(${LIB_NAME} SHARED ${SRC_LIST})
#target_link_libraries(${LIB_NAME} PRIVATE pybind11::module)
TARGET_LINK_LIBRARIES(${LIB_NAME} ${CTPAPI_MD_LIBRARY} ${CTPAPI_TD_LIBRARY} pthread iqt)

add_subdirectory(portal)
#[[if (VBIN)
    add_definitions(-DVBIN)
endif (VBIN)

if (VTEST)
    include_directories(${PROJECT_SOURCE_DIR}/test)
    add_definitions(-DVTEST)
else (VTEST)
    if (NOT VBIN)
        add_subdirectory(portal)
    endif (NOT VBIN)
endif (VTEST)]]
