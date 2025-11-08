add_library(http_client STATIC curl_global.cpp curl_easy.cpp)
target_include_directories(http_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(http_client
        PUBLIC  http_model http_cache utils
        PRIVATE CURL::libcurl
)
