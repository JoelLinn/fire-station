cmake_minimum_required(VERSION 4.0)
include(FetchContent)

project(fire-station VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS FALSE)

include(cmake/fmt.cmake)
include(cmake/SOEM.cmake)

configure_file(Config.h.in include/Config.h)

add_executable(fire-station
        main.cpp
        Format.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h
)
target_include_directories(fire-station PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(fire-station soem)
target_compile_options(fire-station PRIVATE -Wall -Wextra -pedantic)

install(TARGETS fire-station)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fire station automation." CACHE STRING "")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Joel Linn" CACHE STRING "")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
endif()
include(CPack)
