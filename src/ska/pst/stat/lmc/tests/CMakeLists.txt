
include_directories(../..)

add_executable(StatLmcServiceHandlerTest src/StatLmcServiceHandlerTest.cpp)
add_executable(StatLmcServiceIntegrationTest src/StatLmcServiceIntegrationTest.cpp)

set(TEST_LINK_LIBS
    gtest_main
    ska_pst_stat
    ska-pst-stat-testutils
    ${SkaPstCommon_LIBRARIES}
)

target_link_libraries(StatLmcServiceHandlerTest ${TEST_LINK_LIBS})
target_link_libraries(StatLmcServiceIntegrationTest ${TEST_LINK_LIBS})

add_test(StatLmcServiceHandlerTest StatLmcServiceHandlerTest --test_data "${CMAKE_CURRENT_LIST_DIR}/../../tests/data")
add_test(StatLmcServiceIntegrationTest StatLmcServiceIntegrationTest --test_data "${CMAKE_CURRENT_LIST_DIR}/../../tests/data")
