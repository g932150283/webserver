cmake_minimum_required(VERSION 3.0)
project(webserver)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
# 指定编译器的行为
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# include_directories(${PROJECT_SOURCE_DIR}/src)
find_package(Protobuf)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()
set(LIB_SRC
    src/address.cc
    src/log.cc
    src/util.cc
    src/config.cc
    src/thread.cc
    src/mutex.cc
    src/fiber.cc
    src/scheduler.cc
    src/iomanager.cc
    src/timer.cc
    src/env.cc
    src/hook.cc
    src/fd_manager.cc
    src/util/json_util.cc
    )

add_library(webserver SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(webserver) # __FILE__

set(LIBS
         webserver
         pthread
         yaml-cpp
         jsoncpp
         dl
         ${PROTOBUF_LIBRARIES}
    )

message("***", ${LIBS})

# add_executable(test tests/test.cc)
# add_dependencies(test webserver)
# force_redefine_file_macro_for_sources(test)
# target_link_libraries(test ${LIBS})


add_executable(test_tuple tests/test_tuple.cc)
force_redefine_file_macro_for_sources(test_tuple)

add_executable(test_async tests/test_async.cc)
force_redefine_file_macro_for_sources(test_async)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config webserver)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread webserver)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util webserver)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber webserver)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler webserver)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager webserver)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook webserver)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address webserver)
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIBS})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
