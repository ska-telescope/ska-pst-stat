# Find the SkaPstSmrb library
#

# The following are set after configuration is done:
#  SkaPstSmrb_FOUND
#  SkaPstSmrb_INCLUDE_DIR
#  SkaPstSmrb_LIBRARIES

find_path(SkaPstSmrb_INCLUDE_DIR
    NAMES ska/pst/smrb/DataBlock.h
    HINTS
    ${SkaPstSmrb_ROOT_DIR}/include)

find_library(SkaPstSmrb_LIBRARIES
    NAMES ska_pst_smrb
    HINTS
    ${SkaPstSmrb_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SkaPstSmrb DEFAULT_MSG SkaPstSmrb_INCLUDE_DIR SkaPstSmrb_LIBRARIES)
mark_as_advanced(SkaPstSmrb_INCLUDE_DIR SkaPstSmrb_LIBRARIES)