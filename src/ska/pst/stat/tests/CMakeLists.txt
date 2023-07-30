
include_directories(../..)

add_executable(StatHdf5FileWriterTest src/StatHdf5FileWriterTest.cpp)
add_executable(StatStorageTest src/StatStorageTest.cpp)
add_executable(StatComputerTest src/StatComputerTest.cpp)

target_link_libraries(StatHdf5FileWriterTest gtest_main ska_pst_stat ska-pst-stat-testutils)
target_link_libraries(StatStorageTest gtest_main ska_pst_stat ska-pst-stat-testutils)
target_link_libraries(StatComputerTest gtest_main ska_pst_stat ska-pst-stat-testutils)

add_test(StatHdf5FileWriterTest StatHdf5FileWriterTest --test_data "${CMAKE_CURRENT_LIST_DIR}/data")
add_test(StatStorageTest StatStorageTest --test_data "${CMAKE_CURRENT_LIST_DIR}/data")
add_test(StatComputerTest StatComputerTest --test_data "${CMAKE_CURRENT_LIST_DIR}/data")
