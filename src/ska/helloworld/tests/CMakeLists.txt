add_executable(twave twave.cc)
target_link_libraries(twave HelloClasses gtest)
add_test(
    NAME twave
    COMMAND twave
)
