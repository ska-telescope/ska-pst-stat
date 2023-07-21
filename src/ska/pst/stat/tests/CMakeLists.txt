
include_directories(../..)

add_executable(StatHdf5FileWriterTest src/StatHdf5FileWriterTest.cpp)

target_link_libraries(StatHdf5FileWriterTest gtest_main ska_pst_stat ska-pst-stat-testutils)

add_test(StatHdf5FileWriterTest StatHdf5FileWriterTest --test_data "${CMAKE_CURRENT_LIST_DIR}/data")
