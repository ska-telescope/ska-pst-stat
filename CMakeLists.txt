cmake_minimum_required(VERSION 3.5)

# Project configuration, specifying version, languages,
# and the C++ standard to use for the whole project
file(READ ".release" ver)
string(REGEX MATCH "release=([0-9]+.[0-9]+.[0-9]+)" _ ${ver})
set(SkaPstStat_VERSION ${CMAKE_MATCH_1})
set(VERSION ${CMAKE_MATCH_1})

set(PACKAGE_NAME SkaPstStat)

message(STATUS "Building ${PACKAGE_NAME} version ${SkaPstStat_VERSION}")

set(DESCRIPTION "STAT Software for the SKA Pulsar Timing Product")

project(${PACKAGE_NAME} VERSION ${SkaPstStat_VERSION} DESCRIPTION ${DESCRIPTION} LANGUAGES CXX )
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/resources/cmake-modules ${PROJECT_SOURCE_DIR}/cmake)
include(dependencies)
include(lint)
include(CTest)

# External projects
if (BUILD_TESTING)
  find_package(GTest REQUIRED)
endif()

# find_package(psrdada REQUIRED)
# find_package(Protobuf)
# find_package(gRPC)
# find_package(SkaPstLmc)
find_package(SkaPstCommon REQUIRED)
find_package(SkaPstSmrb REQUIRED)
find_package(spdlog REQUIRED)
find_package(HDF5 REQUIRED CXX)

find_dependencies()

# add_subdirectory(src/ska/helloworld)
# add_subdirectory(src/apps)
add_subdirectory(src/ska/pst/stat)

# Install cmake config + version + target files
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/SkaPstStatConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/SkaPstStatConfig.cmake"
  INSTALL_DESTINATION
    cmake/ska_pst_stat
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/SkaPstStatVersion.cmake"
  COMPATIBILITY
    AnyNewerVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/SkaPstStatConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/SkaPstStatVersion.cmake
  DESTINATION
    cmake/ska_pst_stat
  COMPONENT
  dev
)

# Deb Packing
include(Packing)