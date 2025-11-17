add_library(utils STATIC string_utils.cpp thread_pool.cpp money_utils.cpp)
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
