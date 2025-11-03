add_library(http_provider STATIC polygon.cpp)
target_include_directories(http_provider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(http_provider
        PUBLIC  http_model http_api
        PRIVATE simdjson::simdjson
)