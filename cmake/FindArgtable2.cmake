# - Try to find argtable2
#
# The following variables are optionally searched for defaults
#  ARGTABLE2_ROOT_DIR:            Base directory where all ARGTABLE2 components are found
#
# The following are set after configuration is done: 
#  ARGTABLE2_FOUND
#  ARGTABLE2_INCLUDE_DIRS
#  ARGTABLE2_LIBRARIES

include(FindPackageHandleStandardArgs)

set(ARGTABLE2_ROOT_DIR "E:\\src\\vc\\argtable2-13" CACHE PATH "Folder contains argtable2")

find_path(ARGTABLE2_INCLUDE_DIR argtable2.h
    PATHS ${ARGTABLE2_ROOT_DIR}\\src)

find_library(ARGTABLE2_LIBRARY_RELEASE argtable2
    PATHS ${ARGTABLE2_ROOT_DIR}
    PATH_SUFFIXES src)

find_library(ARGTABLE2_LIBRARY_DEBUG argtable2
    PATHS ${ARGTABLE2_ROOT_DIR}
    PATH_SUFFIXES src)

set(ARGTABLE2_LIBRARY optimized ${ARGTABLE2_LIBRARY_RELEASE} debug ${ARGTABLE2_LIBRARY_DEBUG})

find_package_handle_standard_args(ARGTABLE2 DEFAULT_MSG
    ARGTABLE2_INCLUDE_DIR ARGTABLE2_LIBRARY)

if(ARGTABLE2_FOUND)
    set(ARGTABLE2_INCLUDE_DIRS ${ARGTABLE2_INCLUDE_DIR})
    set(ARGTABLE2_LIBRARIES ${ARGTABLE2_LIBRARY})
endif()