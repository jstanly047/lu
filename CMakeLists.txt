cmake_minimum_required(VERSION 3.10.2)
project(joutillib)

option(UNIT_TEST "Build for unit testing" OFF)
set(PROJ_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJ_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

############################################################################
#Include and Headers
############################################################################
include_directories (${PROJ_INC_DIR})


############################################################################
# End Inlcudes and headers
############################################################################

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-std=c++2a -DPROJECT_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra")
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

file (GLOB UTIL_LIB_SRC ./*.cpp)

if (UNIT_TEST)
    add_definitions(-DUNIT_TEST_MODE)
    file(GLOB TEST_QUEUE_SRC ./test/queue/*.cpp)

    add_executable(utillib_test ./test/TestMain.cpp ${TEST_QUEUE_SRC})
    target_link_libraries(utillib_test -static-libgcc
                            libgtest.a)

endif(UNIT_TEST)
unset(UNIT_TEST CACHE)