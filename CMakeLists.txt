cmake_minimum_required(VERSION 3.10.2)
project(LuPlatformLib)
option(UNIT_TEST "Build for unit testing" OFF)
option(COV_BUILD "Flag to enable unit test build" OFF)

set(PROJ_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJ_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJ_TEST_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/mock")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/bin)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
add_definitions(-DPROJECT_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=undefined -fsanitize=address -fsanitize=leak")
#set(CMAKE_CXX_FLAGS_DEBUG "-g  -fsanitize=thread")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=bugprone*,cppcoreguidelines*,concurrency*,hicpp*,performance*,readability*,clang-analyzer*,misc*")
#add_compile_options(-static-libstdc++)
#Check macro expansion
#add_compile_options(-static-libstdc++ -E)


if(COV_BUILD)
  SET(GCC_COVERAGE_COMPILE_FLAGS "-g -O0 -coverage -fprofile-arcs -ftest-coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
  SET(GCC_COVERAGE_LINK_FLAGS    "-coverage -lgcov")
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
  SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )
endif()

find_package(glog REQUIRED)
find_package(gflags REQUIRED)
find_package(libmysqlclient REQUIRED)
find_package(SOCI REQUIRED)

############################################################################
#Build Libraries
############################################################################
file(GLOB UTILS_SRC  ./src/common/*.cpp)
add_library(lucommon STATIC ${UTILS_SRC})
target_include_directories(lucommon PUBLIC ${PROJ_INC_DIR})

file(GLOB UTILS_SRC  ./src/utils/*.cpp)
add_library(utils STATIC ${UTILS_SRC})
target_include_directories(utils PUBLIC ${PROJ_INC_DIR})

file(GLOB LU_PLATFORM_SRC  ./src/platform/*.cpp)
add_library(luPlatform STATIC ${LU_PLATFORM_SRC})
target_include_directories(luPlatform PUBLIC ${PROJ_INC_DIR} ${glog_INCLUDE_DIRS} ${gflags_INCLUDE_DIRS})

file(GLOB LU_SOCKET_DATAH_SRC  ./src/platform/socket/data_handler/*.cpp)
add_library(luSocketDataHandler STATIC ${LU_SOCKET_DATAH_SRC} )
target_include_directories(luSocketDataHandler PUBLIC ${PROJ_INC_DIR})

file(GLOB LU_SOCKET_SRC  ./src/platform/socket/*.cpp)
add_library(luSocket STATIC ${LU_SOCKET_SRC})
target_link_libraries(luSocket luSocketDataHandler luPlatform )

file(GLOB LU_THREAD_CHANNEL_SRC  ./src/platform/thread/channel/*.cpp)
add_library(luThreadChannel STATIC ${LU_THREAD_CHANNEL_SRC})
target_include_directories(luThreadChannel PUBLIC ${PROJ_INC_DIR})

file(GLOB LU_THREAD_SRC  ./src/platform/thread/*.cpp)
add_library(luThread STATIC ${LU_THREAD_SRC})
target_link_libraries(luThread luThreadChannel luSocket)

file(GLOB LU_STORAGE_DB_SRC  ./src/storage/db/*.cpp)
add_library(luStorageDB STATIC ${LU_STORAGE_DB_SRC})
target_link_libraries(luStorageDB SOCI::soci_mysql_static)
target_include_directories(luStorageDB PUBLIC ${PROJ_INC_DIR}
                                              ${SOCI_INCLUDE_DIRS} 
                                              ${libmysqlclient_INCLUDE_DIRS}
                                              ${glog_INCLUDE_DIRS} 
                                              ${gflags_INCLUDE_DIRS})

#file(GLOB THREAD_SRC  ./src/thread/*.cpp)
#add_library(thread STATIC ${THREAD_SRC})
#target_include_directories(thread PUBLIC ${PROJ_INC_DIR})

############################################################################
# Build unit tests
############################################################################
if (UNIT_TEST)
    add_definitions(-DUNIT_TEST_MODE)
    find_package(GTest REQUIRED)

    file(GLOB LU_MOCK_THREAD_SRC  ./test/mock/platform/thread/*.h)
    add_library(luMockThread STATIC ${LU_MOCK_THREAD_SRC})
    target_link_libraries(luMockThread luThread)
    target_include_directories(luMockThread PUBLIC ${PROJ_TEST_INC_DIR} ${LU_PLATFORM_HEADRES})

    file(GLOB TEST_UTILS_SRC ./test/utils/*.cpp)
    file(GLOB TEST_QUEUE_SRC ./test/queue/*.cpp)
    file(GLOB TEST_SOCKET_SRC ./test/platform/socket/*.cpp)
    file(GLOB TEST_THREAD_SRC ./test/platform/thread/*.cpp)
    file(GLOB TEST_PlATFORM_SRC ./test/platform/*.cpp)
    file(GLOB TEST_REFLECTION_SRC ./test/reflection/*.cpp)
    file(GLOB TEST_STORAGE_SRC ./test/storage/*.cpp)

    add_executable(LuTests ./test/TestMain.cpp 
                            ${TEST_UTILS_SRC}
                            ${TEST_QUEUE_SRC}
                            ${TEST_SOCKET_SRC}
                            ${TEST_THREAD_SRC}
                            ${TEST_PlATFORM_SRC}
                            ${TEST_REFLECTION_SRC}
                            ${TEST_STORAGE_SRC})

    target_include_directories(LuTests PRIVATE ${GTEST_INCLUDE_DIRS} ${LU_PLATFORM_HEADRES})

    target_link_libraries(LuTests
                            luPlatform
                            utils
                            #thread
                            luSocketDataHandler
                            luSocket
                            luThread
                            luStorageDB
                            luMockThread
                            glog::glog
                            GTest::gtest
                            GTest::gmock
                            )

endif(UNIT_TEST)
unset(UNIT_TEST CACHE)
unset(COV_BUILD CACHE)