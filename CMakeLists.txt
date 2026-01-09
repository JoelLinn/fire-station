cmake_minimum_required(VERSION 3.28)
include(FetchContent)

project(fire-station VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)

option(FIRE_STATION_ETHERCAT_DUMMY "Disable EtherCat communication and run with dummy data" OFF)

include(cmake/cJSON.cmake)
include(cmake/curl.cmake)
include(cmake/disruptorplus.cmake)
include(cmake/fmt.cmake)
include(cmake/SOEM.cmake)
find_package(OpenSSL REQUIRED)

configure_file(src/BuildConfig.h.in include/BuildConfig.h)

add_executable(fire-station
        src/Alarms.hpp
        src/Announcements.cpp
        src/Announcements.hpp
        src/Announcer.cpp
        src/Announcer.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/BuildConfig.h
        src/Config.cpp
        src/Config.hpp
        src/Controller.cpp
        src/Controller.hpp
        src/Format.hpp
        src/GeneralizedQueue.hpp
        src/Ipc.hpp
        src/PushListener.cpp
        src/PushListener.hpp
        src/RaiiHelpers.hpp
        src/Sha256.cpp
        src/Sha256.hpp
        src/TtsDispatcher.cpp
        src/TtsDispatcher.hpp
        src/main.cpp
        src/$<IF:$<BOOL:${FIRE_STATION_ETHERCAT_DUMMY}>,simple_ng_dummy.cpp,simple_ng.c>
        src/simple_ng.h
)
target_include_directories(fire-station PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(fire-station PRIVATE
        cJSON
        CURL::libcurl
        disruptorplus
        fmt
        OpenSSL::Crypto
        soem
)
target_compile_options(fire-station PRIVATE -Wall -Wextra -pedantic)

install(TARGETS fire-station)

install(FILES data/fire-station DESTINATION /etc PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ)
install(FILES data/fire-station.service DESTINATION lib/systemd/system/)
install(FILES data/GongLong.wav DESTINATION share/${PROJECT_NAME}/)
install(FILES data/GongShort.wav DESTINATION share/${PROJECT_NAME}/)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fire station automation." CACHE STRING "")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/deb/conffiles")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Joel Linn" CACHE STRING "")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
endif()
include(CPack)
