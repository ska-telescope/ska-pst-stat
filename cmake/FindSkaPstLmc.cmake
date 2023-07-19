# Find the SKA-PST-LMC Protobuf Bindings and library
#

# The following are set after configuration is done:
#  SKAPSTLMC_FOUND
#  SKAPSTLMC_INCLUDE_DIR
#  SKAPSTRECV_LMCSERVICE_INCLUDE_DIR
#  SKAPSTLMC_LIBRARIES

find_path(SKAPSTLMC_INCLUDE_DIR
    NAMES 
    ska/pst/lmc/ska_pst_lmc.grpc.pb.h
)



find_library(SKAPSTLMC_LIBRARIES
    NAMES
    ska_pst_lmc
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SkaPstLmc DEFAULT_MSG SKAPSTLMC_INCLUDE_DIR SKAPSTLMC_LIBRARIES)
