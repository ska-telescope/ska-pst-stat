# if ((gRPC_FOUND) AND (PROTOBUF_FOUND) AND (SkaPstLmc_FOUND))
#   set(HAVE_PROTOBUF_gRPC_SKAPSTLMC "ON")
# endif((gRPC_FOUND) AND (PROTOBUF_FOUND) AND (SkaPstLmc_FOUND))

configure_file(config.h.in config.h)

set(public_headers
  version.h
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  StatStorage.h
  StatPublisher.h
  StatHdf5FileWriter.h
  StatComputer.h
  StatProcessor.h
)

set(sources
  src/version.cpp
  src/StatPublisher.cpp
  src/StatHdf5FileWriter.cpp
  src/StatComputer.cpp
  src/StatStorage.cpp
  src/StatProcessor.cpp
)

set(private_headers
)

if (BUILD_TESTING)
add_subdirectory(testutils)
add_subdirectory(tests)
endif()

add_library(ska_pst_stat
  SHARED
    ${sources} ${optional_sources}
    ${private_headers} ${optional_private_headers}
    ${public_headers}
)

# include_directories(${HDF5_INCLUDE_DIRS})

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
    ska_pst_common
    ${PSRDADA_LIBRARIES}
    ${HDF5_CXX_LIBRARIES}
)
