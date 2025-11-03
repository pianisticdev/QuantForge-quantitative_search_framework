cmake_minimum_required(VERSION 3.27)
project(monte_carlo_and_backtester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# External deps
find_package(CURL REQUIRED)
find_package(simdjson CONFIG REQUIRED)

# Your internal libs
add_subdirectory(src/http/model)
add_subdirectory(src/http/error)
add_subdirectory(src/http/cache)
add_subdirectory(src/http/client)
add_subdirectory(src/http/provider)
add_subdirectory(src/http/api)
add_subdirectory(src/utils)

# Executable that wires everything up in main.cpp
add_executable(monte_carlo_and_backtester main.cpp)

# Only link the top-level API; it brings the rest transitively.
target_link_libraries(monte_carlo_and_backtester
        PRIVATE
        http_api
        http_provider
        utils
)