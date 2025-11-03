cmake_minimum_required(VERSION 3.24)
project(btree-raylib VERSION 0.1 LANGUAGES C CXX)

# Generate compile_commands.json for editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(RAYLIB_VERSION 5.5)

# Enable FetchContent caching
set(FETCHCONTENT_QUIET FALSE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/raylib-src
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/raylib-build
)

# Set platform for Raylib
if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "Platform" FORCE)
endif()

FetchContent_MakeAvailable(raylib)

# Generate embedded font header
set(FONT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/JetBrainsMono-Regular.ttf")
set(FONT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/embedded_font.h")

add_custom_command(
    OUTPUT ${FONT_HEADER}
    COMMAND ${CMAKE_COMMAND} 
        -DINPUT_FILE=${FONT_FILE}
        -DOUTPUT_FILE=${FONT_HEADER}
        -DVARIABLE_NAME=embedded_font_data
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_file.cmake
    DEPENDS ${FONT_FILE}
    COMMENT "Embedding font file into header"
)

# Collect sources from src/ and build a single executable.
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c)
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS src/*.hpp src/*.h)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${FONT_HEADER})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Web-specific settings (Emscripten)
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".html"
    )
    
    # Emscripten link flags for better web experience
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3
        -sASSERTIONS=1
        -sWASM=1
        -sASYNCIFY
        -sALLOW_MEMORY_GROWTH=1
        --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/src/resources@/resources
    )
    
    # Set initial memory (16MB)
    target_link_options(${PROJECT_NAME} PRIVATE -sINITIAL_MEMORY=16777216)
    
    # Copy additional web files to build directory
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html
        ${CMAKE_BINARY_DIR}/bin/index.html
        COPYONLY
    )
endif()

# On Windows, hide the console window without changing entry point
if(WIN32 AND MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)