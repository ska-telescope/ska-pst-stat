cmake_minimum_required(VERSION 3.5)

message(STATUS "Building ...")

# Project configuration, specifying version, languages,
# and the C++ standard to use for the whole project
file(READ ".release" ver)
string(REGEX MATCH "release=([0-9]+.[0-9]+.[0-9]+)" _ ${ver})

set(PACKAGE_NAME SkaPstStat)
set(VERSION ${CMAKE_MATCH_1})
set(DESCRIPTION "STAT Software for the SKA Pulsar Timing Product")

project(${PACKAGE_NAME} VERSION ${VERSION} DESCRIPTION ${DESCRIPTION} LANGUAGES CXX )
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/resources/cmake-modules)
include(dependencies)
include(lint)
include(CTest)

# External projects
if (BUILD_TESTING)
  find_package(GTest REQUIRED)
endif()

find_dependencies()

add_subdirectory(src/ska/helloworld)
add_subdirectory(src/apps)

# Install cmake config + version + target files
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/HelloWorldConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/HelloWorldConfig.cmake"
  INSTALL_DESTINATION
    share/helloworld/cmake
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/HelloWorldVersion.cmake"
  COMPATIBILITY
    AnyNewerVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/HelloWorldConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/HelloWorldVersion.cmake
  DESTINATION
    share/helloworld/cmake
  COMPONENT
    dev
)
install(EXPORT helloworld-targets
  FILE
    HelloWorldTargets.cmake
  DESTINATION
    share/helloworld/cmake
  COMPONENT
    dev
  NAMESPACE
    helloworld::
)

# Deb Packing
include(Packing)