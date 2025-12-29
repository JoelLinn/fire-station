message(STATUS "====== Fetching disruptorplus ======")

FetchContent_Declare(
        disruptorplus
        GIT_REPOSITORY https://github.com/xenia-project/disruptorplus
        GIT_TAG 5787e9cb7551c8c79cf9ce14f7be860dc907e9a4 # master branch
        SOURCE_SUBDIR "DO_NOT_CALL_ADD_SUBDIRECTORY"
)

FetchContent_MakeAvailable(disruptorplus)

add_library(disruptorplus INTERFACE)
target_include_directories(disruptorplus INTERFACE ${disruptorplus_SOURCE_DIR}/include)
