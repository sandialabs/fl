# - Find FFTW library
# Find the native FFTW includes and library
# This module defines
#   FFTW_FOUND         If false, do not try to use FFTW.
#   FFTW_INCLUDE_DIRS  Directory containing the include files
#   FFTW_LIBRARIES     Full path to library to link against

find_path (FFTW_INCLUDE_DIRS
  NAMES fftw3.h
  PATH_SUFFIXES fftw
)

find_library (FFTW_DOUBLE_LIB
  NAMES fftw3 fftw2xf_intel
  PATHS ENV FFTW_LIB
)

find_library (FFTW_FLOAT_LIB
  NAMES fftw3f
  PATHS ENV FFTW_LIB
)

find_library (FFTW_LONG_LIB
  NAMES fftw3l
  PATHS ENV FFTW_LIB
)

set (FFTW_LIBRARIES)
if (FFTW_DOUBLE_LIB)
  set (FFTW_LIBRARIES ${FFTW_LIBRARIES} ${FFTW_DOUBLE_LIB})
endif (FFTW_DOUBLE_LIB)
if (FFTW_FLOAT_LIB)
  set (FFTW_LIBRARIES ${FFTW_LIBRARIES} ${FFTW_FLOAT_LIB})
endif (FFTW_FLOAT_LIB)
if (FFTW_LONG_LIB)
  set (FFTW_LIBRARIES ${FFTW_LIBRARIES} ${FFTW_LONG_LIB})
endif (FFTW_LONG_LIB)

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFTW  DEFAULT_MSG  FFTW_LIBRARIES FFTW_INCLUDE_DIRS)

mark_as_advanced (
  FFTW_INCLUDE_DIRS
  FFTW_LIBRARIES
  FFTW_DOUBLE_LIB
  FFTW_FLOAT_LIB
  FFTW_LONG_LIB
)
