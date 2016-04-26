# Check for cmake minimum required version (used by development)
cmake_minimum_required(VERSION 2.8)
message(STATUS "CMake 2.8+: ${CMAKE_VERSION}")

# Set project header
project(FLuaG)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
set(PROJECT_AUTHOR Youka)
set(PROJECT_DESCRIPTION "Frameserver plugin for script based video editing.")
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
message(STATUS "Project header: '${PROJECT_NAME}' by ${PROJECT_AUTHOR}, v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Enable tests (must be here for target generation)
message(STATUS "Enable tests...")
enable_testing()

# Process source
message(STATUS "Process source...")
add_subdirectory(src)

# Process documents
message(STATUS "Process documents...")
add_subdirectory(docs)
