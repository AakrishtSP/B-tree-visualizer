cmake_minimum_required(VERSION 3.24)
project(btree-raylib VERSION 0.1 LANGUAGES C CXX)

# Generate compile_commands.json for editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(RAYLIB_VERSION 5.0)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
)
FetchContent_MakeAvailable(raylib)

# Collect sources from src/ and build a single executable.
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c)
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS src/*.hpp src/*.h)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)