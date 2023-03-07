cmake_minimum_required(VERSION 3.10.2)
project(joutillib)

option(UNIT_TEST "Build for unit testing" OFF)
set(PROJ_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJ_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/bin)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_compile_options(-static-libstdc++)


############################################################################
#Build Libraries
############################################################################
file(GLOB UTILS_SRC  ./src/utility/*.cpp)
add_library(utils STATIC ${UTILS_SRC})
target_include_directories(utils PUBLIC ${PROJ_INC_DIR})

#file(GLOB QUEUE_SRC  ./src/queue/*.cpp)
#add_library(queue STATIC ${QUEUE_SRC})
#target_include_directories(queue PUBLIC ${PROJ_INC_DIR})

file(GLOB SOCKET_SRC  ./src/socket/*.cpp)
add_library(socket STATIC ${SOCKET_SRC})
target_include_directories(socket PUBLIC ${PROJ_INC_DIR})

file(GLOB THREAD_SRC  ./src/thread/*.cpp)
add_library(thread STATIC ${THREAD_SRC})
target_include_directories(thread PUBLIC ${PROJ_INC_DIR})


############################################################################
# Build unit tests
############################################################################
if (UNIT_TEST)
    add_definitions(-DUNIT_TEST_MODE)
    file(GLOB TEST_UTILS ./test/utils/*.cpp)
    file(GLOB TEST_QUEUE ./test/queue/*.cpp)
    file(GLOB TEST_THREAD ./test/thread/*.cpp)

add_executable(LuTests ./test/TestMain.cpp 
                        ${TEST_UTILS}
                        ${TEST_QUEUE}
                        ${TEST_THREAD})

target_link_libraries(LuTests
                        utils
                        #queue
                        thread
                        -lgtest
                        -lgmock
                        -pthread)

endif(UNIT_TEST)
unset(UNIT_TEST CACHE)