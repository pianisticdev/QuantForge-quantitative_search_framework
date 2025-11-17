add_library(plugins_manifest STATIC manifest.cpp)
target_include_directories(plugins_manifest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(plugins_manifest PRIVATE simdjson::simdjson utils)
