cmake_minimum_required(VERSION 3.20)

project(ArchiveFolders
    VERSION 1.0
    DESCRIPTION "A FUSE filesystem for archive formats"
    LANGUAGES CXX
)

# This generates 'compile_commands.json' in the build directory.
# clangd (and other LSPs) use this to understand your project structure.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # Define paths for the source and destination
    set(SOURCE_CC "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(TARGET_CC "${CMAKE_SOURCE_DIR}/compile_commands.json")

    # Create the symlink automatically during the CMake configuration step
    # 'create_symlink' is cross-platform but works best on Unix-like systems
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${SOURCE_CC} ${TARGET_CC})
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# It's often better to list files explicitly than using globbing.
set(SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Needed by fuse on linux
find_package(PkgConfig REQUIRED)

# Libraries
find_package(libzip QUIET)
pkg_check_modules(FUSE3 REQUIRED IMPORTED_TARGET fuse3)

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::FUSE3)

if(NOT libzip_FOUND)
    pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBZIP)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC libzip::zip)
endif()

# Include directory setup
target_include_directories(${PROJECT_NAME} PUBLIC src/include)

# This adds a 'make format' (or 'ninja format') command.
find_program(CLANG_FORMAT_EXE NAMES clang-format)

if(CLANG_FORMAT_EXE)
    # Get all project files to format
    file(GLOB_RECURSE ALL_SOURCE_FILES
        src/*.cpp
        src/*.hpp
        include/*.hpp
    )

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCE_FILES}
        COMMENT "Formatting project source files with clang-format..."
    )
else()
    message(WARNING "clang-format not found. The 'format' target will not be available.")
endif()
