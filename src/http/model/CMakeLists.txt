add_library(http_model INTERFACE model.cpp)
target_include_directories(http_model INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(http_model PUBLIC)