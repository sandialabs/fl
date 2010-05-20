# - Find FFTW library
# Find the native FFTW includes and library
# This module defines
#   FFTW_FOUND         If false, do not try to use FFTW.
#   FFTW_INCLUDE_DIRS  Directory containing the include files
#   FFTW_LIBRARIES     Full path to library to link against

find_path (FFTW_INCLUDE_DIRS fftw3.h)

find_library (FFTW_LIBRARIES fftw3)

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFTW  DEFAULT_MSG  FFTW_LIBRARIES FFTW_INCLUDE_DIRS)

mark_as_advanced (
  FFTW_INCLUDE_DIRS
  FFTW_LIBRARIES
)
