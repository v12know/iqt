SET(LIB_NAME portal_ctp)

#include_directories(${PROJECT_SOURCE_DIR}/extra)
#include_directories(${PROJECT_SOURCE_DIR}/include)
#include_directories(${PROJECT_SOURCE_DIR}/src)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-fvisibility=hidden ${CMAKE_CXX_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCXX)
#aux_source_directory(base BASE_LIST)
#aux_source_directory(ctp CTP_LIST)
#aux_source_directory(stgy STGY_LIST)
#aux_source_directory(GLOB_RECURSE . SRC_LIST)
#file(GLOB_RECURSE SRC_LIST "*.cpp")

SET(SRC_LIST
        portal_ctp.cpp
		portal_ctp.h
)

message("SRC_LIST=${SRC_LIST}")

#find_library(IQT_LIBRARY NAMES iqt PATHS ${PROJECT_SOURCE_DIR}/lib)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/pyiqt/ctp)

add_library(${LIB_NAME} SHARED ${SRC_LIST})
#add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/portal portal.out)
#pybind11_add_module(${LIB_NAME} SHARED ${SRC_LIST})
set_target_properties(${LIB_NAME} PROPERTIES PREFIX "")
target_link_libraries(${LIB_NAME} PRIVATE pybind11::module ctp)
#TARGET_LINK_LIBRARIES(${LIB_NAME} PRIVATE ctp)
