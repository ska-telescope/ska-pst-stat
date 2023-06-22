# Configure a header file to pass some of the CMake settings
# to the source code
configure_file(config.h.in config.h)

set(public_headers
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  hello.h
  wave.h
)
set(private_headers
  wave_impl.h
)
set(sources
  hello.cc
  wave.cc
)

add_library(
  HelloClasses
    ${sources}
    ${private_headers}
    ${public_headers}
)
target_include_directories(HelloClasses PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>
)
add_library(helloworld::HelloClasses ALIAS HelloClasses)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(
  TARGETS
    HelloClasses
  EXPORT
    helloworld-targets
  LIBRARY DESTINATION
    lib
  ARCHIVE DESTINATION
    lib
)

install(
  FILES
    ${public_headers}
  DESTINATION
    include/ska/helloworld
)
