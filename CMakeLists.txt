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
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
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
    src/http/http.cc
    src/http/http_connection.cc
    src/http/http_parser.cc
    src/http/http_session.cc
    src/http/http_server.cc
    src/http/servlet.cc
    src/http/servlets/config_servlet.cc
    src/http/servlets/status_servlet.cc
    src/http/session_data.cc
    src/http/ws_connection.cc
    src/http/ws_session.cc
    src/http/ws_server.cc
    src/http/ws_servlet.cc
    src/hook.cc
    src/fd_manager.cc
    src/library.cc
    src/util/crypto_util.cc
    src/util/json_util.cc
    src/util/hash_util.cc
    src/socket.cc
    src/bytearray.cc
    src/stream.cc
    src/streams/async_socket_stream.cc
    src/streams/socket_stream.cc
    src/streams/load_balance.cc
    src/streams/service_discovery.cc
    src/streams/zlib_stream.cc
    src/tcp_server.cc
    src/zk_client.cc
    src/worker.cc
    src/module.cc
    src/rock/rock_protocol.cc
    src/rock/rock_server.cc
    src/rock/rock_stream.cc
    src/protocol.cc
    src/daemon.cc
    src/application.cc
    )

ragelmaker(src/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(webserver SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(webserver) # __FILE__

set(LIBS
         webserver
         dl
         pthread
         yaml-cpp
         jsoncpp
         ${OPENSSL_LIBRARIES}
         ${PROTOBUF_LIBRARIES}         
         hiredis_vip
         zookeeper_mt
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

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket webserver)
force_redefine_file_macro_for_sources(test_socket)
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray webserver)
force_redefine_file_macro_for_sources(test_bytearray)
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http webserver)
force_redefine_file_macro_for_sources(test_http)
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser webserver)
force_redefine_file_macro_for_sources(test_http_parser)
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server webserver)
force_redefine_file_macro_for_sources(test_tcp_server)
target_link_libraries(test_tcp_server ${LIBS})


add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server webserver)
force_redefine_file_macro_for_sources(echo_server)
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server webserver)
force_redefine_file_macro_for_sources(test_http_server)
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection webserver)
force_redefine_file_macro_for_sources(test_http_connection)
target_link_libraries(test_http_connection ${LIBS})

add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri webserver)
force_redefine_file_macro_for_sources(test_uri)
target_link_libraries(test_uri ${LIBS})


add_executable(my_http_server samples/my_http_server.cc)
add_dependencies(my_http_server webserver)
force_redefine_file_macro_for_sources(my_http_server)
target_link_libraries(my_http_server ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
