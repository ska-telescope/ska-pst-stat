# Find the SkaPstCommon library
#

# The following are set after configuration is done:
#  SkaPstCommon_FOUND
#  SkaPstCommon_INCLUDE_DIR
#  SkaPstCommon_LIBRARIES

find_path(SkaPstCommon_INCLUDE_DIR
    NAMES ska/pst/common/statemodel/StateModel.h
    HINTS
    ${SkaPstCommon_ROOT_DIR}/include)

find_library(SkaPstCommon_LIBRARIES
    NAMES ska_pst_common
    HINTS
    ${SkaPstCommon_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SkaPstCommon DEFAULT_MSG SkaPstCommon_INCLUDE_DIR SkaPstCommon_LIBRARIES)
mark_as_advanced(SkaPstCommon_INCLUDE_DIR SkaPstCommon_LIBRARIES)