cmake_minimum_required(VERSION 3.10)
project(alib-cpu)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

set(SRC_DIR ${CMAKE_SOURCE_DIR})
set(INCLUDE_DIR ${SRC_DIR}/include)
set(DSDK_INCLUDE_DIR ${SRC_DIR}/dsdk/include)
set(DSDK_LIB_DIR ${SRC_DIR}/dsdk/lib)

include_directories(
    ${INCLUDE_DIR}
    ${DSDK_INCLUDE_DIR}
)

set(SOURCES
    ${SRC_DIR}/alib_cpu.cpp
)

add_library(alibcpu STATIC ${SOURCES})
# add_library(alibcpu SHARED ${SOURCES})

target_compile_options(alibcpu PRIVATE -fPIC)

target_link_libraries(alibcpu ${DSDK_LIB_DIR}/libasock.a)

install(TARGETS alibcpu DESTINATION lib64)

install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION include)

