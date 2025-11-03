set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(http_cache STATIC network_cache.cpp)
target_include_directories(http_cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(http_cache PUBLIC http_model utils)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_link_libraries(http_cache PUBLIC stdc++fs)
endif()