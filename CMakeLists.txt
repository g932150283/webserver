cmake_minimum_required(VERSION 3.0)
project(webserver)


set(CMAKE_VERBOSE_MAKEFILE ON)
# 指定编译器的行为
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

# include_directories(${PROJECT_SOURCE_DIR}/src)

set(LIB_SRC
    src/log.cc
    src/util.cc
    src/config.cc
    )

add_library(webserver SHARED ${LIB_SRC})

add_executable(test tests/test.cc)
add_dependencies(test webserver)
target_link_libraries(test webserver)

add_executable(test_tuple tests/test_tuple.cc)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config webserver)
target_link_libraries(test_config webserver yaml-cpp)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
