add_library(http_error STATIC http_error.cpp)
target_include_directories(http_error PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)