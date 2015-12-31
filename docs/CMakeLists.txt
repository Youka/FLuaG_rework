# Setup documents installation
message(STATUS "Documents installation path: ${CMAKE_INSTALL_PREFIX}/share/fluag")
include(${PROJECT_SOURCE_DIR}/cmake/InstallEx.cmake)
installex("*" "share/fluag")