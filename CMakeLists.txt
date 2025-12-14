cmake_minimum_required(VERSION 3.27)
project(quant_forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External deps
find_package(CURL REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# HTTP
add_subdirectory(src/http/model)
add_subdirectory(src/http/error)
add_subdirectory(src/http/cache)
add_subdirectory(src/http/client)
add_subdirectory(src/http/provider)
add_subdirectory(src/http/api)
add_subdirectory(src/utils)

# Plugins
add_subdirectory(src/plugins/abi)
add_subdirectory(src/plugins/loaders)
add_subdirectory(src/plugins/manifest)

# Simulators
add_subdirectory(src/simulators/back_test)
add_subdirectory(src/simulators/monte_carlo)

# Renderers
add_subdirectory(src/renderers)

# Forge
add_subdirectory(src/forge/forge_engine)
add_subdirectory(src/forge/stores)

# Executable that wires everything up in main.cpp
add_executable(quant_forge main.cpp)

# Only link the top-level API; it brings the rest transitively.
target_link_libraries(
  quant_forge
  PRIVATE http_api http_provider forge_engine utils)
