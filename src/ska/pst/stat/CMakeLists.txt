if ((gRPC_FOUND) AND (PROTOBUF_FOUND) AND (SkaPstLmc_FOUND))
  set(HAVE_PROTOBUF_gRPC_SKAPSTLMC "ON")
endif((gRPC_FOUND) AND (PROTOBUF_FOUND) AND (SkaPstLmc_FOUND))

configure_file(config.h.in config.h)

set(public_headers
  version.h
)

set(sources
  src/version.cpp
  src/FileProcessor.cpp
  src/StatApplicationManager.cpp
  src/version.cpp
  src/StatStorage.cpp
  src/StatPublisher.cpp
  src/StatHdf5FileWriter.cpp
  src/StatComputer.cpp
  src/StatStorage.cpp
  src/StatProcessor.cpp
  src/ScalarStatPublisher.cpp
)

set(private_headers
  FileProcessor.h
  StatApplicationManager.h
  StatComputer.h
  StatHdf5FileWriter.h
  StatProcessor.h
  StatPublisher.h
  StatStorage.h
  ScalarStatPublisher.h
)


if (HAVE_PROTOBUF_gRPC_SKAPSTLMC)
  add_subdirectory(lmc)
  add_library(ska_pst_stat
    SHARED
    ${sources} ${optional_sources}
    ${private_headers} ${optional_private_headers}
    ${public_headers}
    $<TARGET_OBJECTS:ska-pst-stat-lmc>
  )
else (HAVE_PROTOBUF_gRPC_SKAPSTLMC)
  add_library(ska_pst_stat
    SHARED
    ${sources} ${optional_sources}
    ${private_headers} ${optional_private_headers}
    ${public_headers}
  )
endif(HAVE_PROTOBUF_gRPC_SKAPSTLMC)

target_include_directories(ska_pst_stat
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
    $<INSTALL_INTERFACE:include>
    ${PSRDADA_INCLUDE_DIR}
    ${HDF5_INCLUDE_DIRS}
)

target_link_libraries(ska_pst_stat
  PUBLIC
    spdlog::spdlog
    ${SkaPstCommon_LIBRARIES}
    ${SkaPstSmrb_LIBRARIES}
    ${PSRDADA_LIBRARIES}
    ${HDF5_CXX_LIBRARIES}
)

if (HAVE_PROTOBUF_gRPC_SKAPSTLMC)
  target_link_libraries(ska_pst_stat
    PUBLIC
      protobuf::libprotobuf
      gRPC::grpc
      gRPC::grpc++
      ${SkaPstLmc_LIBRARIES}
  )
endif(HAVE_PROTOBUF_gRPC_SKAPSTLMC)

if (BUILD_TESTING)
  add_subdirectory(testutils)
  add_subdirectory(tests)
endif()

install(
  TARGETS
    ska_pst_stat
  EXPORT
    ska_pst_stat-targets
  LIBRARY DESTINATION
    lib
  ARCHIVE DESTINATION
    lib
)

install(
  FILES
    ${public_headers}
  DESTINATION
    include/ska/pst/stat
)
