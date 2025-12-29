include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

cmake_push_check_state()
check_cxx_source_compiles("
  #include <format>
  #include <iostream>
  int main(int argc, char *argv[]) {
    auto str = std::format(\"{}, {}\", \"hello\", \"world\");
    std::cout << str << std::endl;
    return 0;
  }" COMPILER_SUPPORTS_STD_FORMAT)
cmake_pop_check_state()
if(COMPILER_SUPPORTS_STD_FORMAT)
    set(HAVE_STD_FORMAT TRUE)
endif()

if(HAVE_STD_FORMAT)
    message(STATUS "====== Using std::format ======")
    add_library(fmt INTERFACE)
else()
    message(STATUS "====== Fetching fmtlib ======")
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281 # 10.2.1
    )

    set(FMT_INSTALL OFF)
    
    FetchContent_MakeAvailable(fmt)
    
    set(fmt_LICENSE_FILE ${fmt_SOURCE_DIR}/LICENSE)
    list(APPEND TACHYON_thirdparty_libs fmt)
endif()