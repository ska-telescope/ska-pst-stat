set(public_headers)

set(sources
  src/StatLmcServiceHandler.cpp
)

set(private_headers
  StatLmcServiceHandler.h
)

add_library(ska-pst-stat-lmc
  OBJECT
    ${sources}
    ${public_headers}
    ${private_headers}
)

target_include_directories(
  ska-pst-stat-lmc
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>
  ${HDF5_INCLUDE_DIRS}
)

target_link_libraries(
  ska-pst-stat-lmc
  PUBLIC
  spdlog::spdlog
  protobuf::libprotobuf
  gRPC::grpc
  gRPC::grpc++
  ${HDF5_CXX_LIBRARIES}
  ${SkaPstLmc_LIBRARIES}
)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(
  FILES
    ${public_headers}
  DESTINATION
    include/ska/pst/stat/lmc
)
