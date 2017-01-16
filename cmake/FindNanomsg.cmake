# - Try to find argtable2
#
# The following variables are optionally searched for defaults
#  NANOMSG_ROOT_DIR:            Base directory where all NANOMSG components are found
#
# The following are set after configuration is done: 
#  NANOMSG_FOUND
#  NANOMSG_INCLUDE_DIRS
#  NANOMSG_LIBRARIES

include(FindPackageHandleStandardArgs)

set(NANOMSG_ROOT_DIR "E:\\src\\vc\\nanomsg-1.0.0-rc1" CACHE PATH "Folder contains nanomsg")

find_path(NANOMSG_INCLUDE_DIR nn.h
    PATHS ${NANOMSG_ROOT_DIR}\\src)

find_library(NANOMSG_LIBRARY_RELEASE nanomsg
    PATHS ${NANOMSG_ROOT_DIR}
    PATH_SUFFIXES build\\Release)

find_library(NANOMSG_LIBRARY_DEBUG nanomsg
    PATHS ${NANOMSG_ROOT_DIR}
    PATH_SUFFIXES build\\Debug)

set(NANOMSG_LIBRARY optimized ${NANOMSG_LIBRARY_RELEASE} debug ${NANOMSG_LIBRARY_DEBUG})

find_package_handle_standard_args(NANOMSG DEFAULT_MSG
    NANOMSG_INCLUDE_DIR NANOMSG_LIBRARY)

if(NANOMSG_FOUND)
    set(NANOMSG_INCLUDE_DIRS ${NANOMSG_INCLUDE_DIR})
    set(NANOMSG_LIBRARIES ${NANOMSG_LIBRARY})
endif()