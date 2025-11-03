add_library(http_api STATIC stock_api.cpp)
target_include_directories(http_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(http_api
        PUBLIC http_model http_client http_error
)