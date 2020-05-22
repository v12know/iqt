cmake_minimum_required (VERSION 3.0)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "set build type to release")
endif()
#set(CMAKE_BUILD_TYPE Debug CACHE STRING "set build type to debug" FORCE)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release" FORCE)

project (iqt CXX)
set(CMAKE_CXX_STANDARD 17)

SET(GTEST on)
SET( CMAKE_VERBOSE_MAKEFILE on )
#SET( PYBIND11_PYTHON_VERSION "3.6" )
SET( PYTHON_EXECUTABLE "/Users/carl/anaconda3/envs/vnpy/bin/python3" )
option( GBK "charset ecoding gbk" off )

message("GBK=${GBK}")

# The version number.
set (iqt_VERSION_MAJOR 1)
set (iqt_VERSION_MINOR 0)

message("PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
message("PROJECT_BIN_DIR=${PROJECT_BIN_DIR}")

add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/pybind11 pybind11.out)
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/cpp_redis cpp_redis.out)

if (GTEST)
	add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/googletest/googletest gtest.out)
endif(GTEST)

#判断编译器类型,如果是gcc编译器,则在编译选项中加入c++11支持
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -g")
	#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	#set(CMAKE_CXX_FLAGS_RELEASE "-fvisibility=hidden ${CMAKE_CXX_FLAGS_RELEASE}")
	#set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wno-format")
	if (GBK)
		set(CMAKE_CXX_FLAGS "-fexec-charset=GBK ${CMAKE_CXX_FLAGS}")
		#add_definitions("-fexec-charset=GBK")
	endif(GBK)
	#add_compile_option(-std=c++11)
	set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "")
	set(CMAKE_SHARED_LINKER_FLAGS "-fPIC -flto -rdynamic")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
	set(CMAKE_EXE_LINKER_FLAGS "-fPIC -flto -rdynamic")
endif(CMAKE_COMPILER_IS_GNUCXX)
#set(PYTHON_MODULE_PREFIX "")
#set(PYTHON_MODULE_EXTENSION "")
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/third-party/portal/tools")

###
# variables
###
set(PROJ_INCLUDES ${PROJECT_SOURCE_DIR}/src)
set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/cereal/include)
#set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/libuv/include)
set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/cpp_redis/includes)
set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/cpp_redis/tacopie/includes)
set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/pybind11/include)
set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/spdlog/include)
set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/tradeapi/linux64)

if (GTEST)
	set(DEPS_INCLUDES ${DEPS_INCLUDES} ${PROJECT_SOURCE_DIR}/third-party/googletest/googletest/include)
endif(GTEST)

set(DEPS_LIBRARIES ${PROJECT_SOURCE_DIR}/lib)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib)
###
# includes
###
include_directories(${PROJ_INCLUDES} ${DEPS_INCLUDES})
add_subdirectory(src)


if (GTEST)
	add_subdirectory(gtest)
endif(GTEST)


#add_subdirectory(extra)
