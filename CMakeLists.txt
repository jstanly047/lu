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

add_definitions(-std=c++2a -DPROJECT_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_COMPILER /usr/bin/g++)

file (GLOB UTIL_LIB_SRC ./*.cpp)

if (UNIT_TEST)
    add_definitions(-DUNIT_TEST_MODE)
    #file(GLOB TEST_QUEUE ./test/*.cpp)

    add_executable(utillib_test ./test/TestMain.cpp
                            ./test/queue/SPSCQueueTest.cpp)
    target_link_libraries(utillib_test -static-libgcc
                            libgtest.a)

endif(UNIT_TEST)
unset(UNIT_TEST CACHE)