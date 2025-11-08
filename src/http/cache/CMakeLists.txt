add_library(http_cache STATIC network_cache.cpp)
target_include_directories(http_cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(http_cache PUBLIC http_model utils)
