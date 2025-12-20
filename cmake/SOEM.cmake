message(STATUS "====== Fetching SOEM ======")

FetchContent_Declare(
        SOEM
        URL https://github.com/OpenEtherCATsociety/SOEM/archive/a7c74cea13786426929ae71e94195fa91c4b9faf.tar.gz # main branch
        URL_HASH SHA512=7b78e6707044276479226d3417a6f84b3a983e4e82c1765ef8a8ea61589bd090e638352d2ba2434b0f4614e685053a7d3e26b48c963e32a5168a9b451358f43c
        OVERRIDE_FIND_PACKAGE
)

set(SOEM_BUILD_SAMPLES OFF)

FetchContent_MakeAvailable(SOEM)
