
enable_testing()

add_library(ska-pst-stat-testutils
    STATIC
    src/GtestMain.cpp)

target_include_directories(ska-pst-stat-testutils
    PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(ska-pst-stat-testutils GTest::gtest_main spdlog::spdlog)

include_directories(..)

#install(FILES GtestMain.hpp)
