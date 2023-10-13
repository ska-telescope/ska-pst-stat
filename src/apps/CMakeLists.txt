
include_directories(
  ..
  ${SkaPstCommon_INCLUDE_DIR}
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

add_executable(ska_pst_stat_core ska_pst_stat_core.cpp)
add_executable(ska_pst_stat_file_proc ska_pst_stat_file_proc.cpp)
add_executable(ska_pst_stat_info ska_pst_stat_info.cpp)

target_link_libraries(ska_pst_stat_core PUBLIC ska_pst_stat)
target_link_libraries(ska_pst_stat_file_proc PUBLIC ska_pst_stat)
target_link_libraries(ska_pst_stat_info PUBLIC ska_pst_stat)

install (
  TARGETS
    ska_pst_stat_core
    ska_pst_stat_file_proc
    ska_pst_stat_info
  DESTINATION
    bin
)

add_test(NAME ska_pst_stat_core COMMAND ska_pst_stat_core -h)
add_test(NAME ska_pst_stat_file_proc COMMAND ska_pst_stat_file_proc -h)
add_test(NAME ska_pst_stat_info COMMAND ska_pst_stat_file_proc -h)

