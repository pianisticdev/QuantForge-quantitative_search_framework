cmake_minimum_required(VERSION 3.27)
project(quant_forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External deps
find_package(CURL REQUIRED)
find_package(simdjson CONFIG REQUIRED)
# find_package(pybind11 CONFIG REQUIRED)

# HTTP
add_subdirectory(src/http/model)
add_subdirectory(src/http/error)
add_subdirectory(src/http/cache)
add_subdirectory(src/http/client)
add_subdirectory(src/http/provider)
add_subdirectory(src/http/api)
add_subdirectory(src/utils)

# Plugins add_subdirectory(src/plugins/abi)
# add_subdirectory(src/plugins/loaders) add_subdirectory(src/plugins/manager)

# Executable that wires everything up in main.cpp
add_executable(quant_forge main.cpp)

# Only link the top-level API; it brings the rest transitively.
target_link_libraries(
  quant_forge
  PRIVATE http_api http_provider
          # plugin_manager
          utils)
