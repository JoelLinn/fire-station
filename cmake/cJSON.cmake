message(STATUS "====== Fetching cJSON ======")

FetchContent_Declare(
        cJSON
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
        GIT_TAG c859b25da02955fef659d658b8f324b5cde87be3 # v1.7.19
        SOURCE_SUBDIR "DO_NOT_CALL_ADD_SUBDIRECTORY"
)

FetchContent_MakeAvailable(cJSON)

set(cJSON_SOURCES
        ${cjson_SOURCE_DIR}/cJSON.c
        ${cjson_SOURCE_DIR}/cJSON_Utils.c)

add_library(cJSON STATIC ${cJSON_SOURCES})

target_include_directories(cJSON PUBLIC ${cjson_SOURCE_DIR})
