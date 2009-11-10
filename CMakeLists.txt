# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


project (FL)
cmake_minimum_required (VERSION 2.6)


# Gather up optional support libraries ----------------------------------------

# Prioritize /usr ahead of / when searching for libraries.  Work-around for
# interaction between encap symbolic links and directory shadowing under Cygwin.
set (CMAKE_PREFIX_PATH /usr)

set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
if (MSVC)
  set (CMAKE_INCLUDE_PATH mswin/include)
  if (CMAKE_CL_64)
    set (CMAKE_LIBRARY_PATH mswin/lib64)
  else (CMAKE_CL_64)
    set (CMAKE_LIBRARY_PATH mswin/lib)
  endif (CMAKE_CL_64)

  # Add libraries under MSVC that are automatically linked by GCC compiler.
  find_library (GCC_LIB gcc)
  find_library (MINGWEX_LIB mingwex)
  find_library (FORTRAN_LIB g2c gfortran)
  set (GCC_LIBRARIES
    ${GCC_LIB}
    ${MINGWEX_LIB}
    ${FORTRAN_LIB}
  )

  find_library (THREAD_LIB pthreadVC2)
  set          (SOCKET_LIB ws2_32)

  # Some includes are available by default under Unix but are not available
  # under MSVC, so add the supplemental include directory pre-emtively.
  include_directories (${PROJECT_SOURCE_DIR}/mswin/include)

else (MSVC)

  # VC Express chokes on CMake's Fortran test, so only do it under a non-VC platform.
  enable_language (Fortran OPTIONAL)

  # CMake prefers gfortran over g77, but unfortunately that breaks my Cygwin setup.
  # Move up to gfortran ASAP, and then get rid of this next line.
  if (CYGWIN)
    set (CMAKE_Fortran_COMPILER g77)
  endif (CYGWIN)

  if (CMAKE_Fortran_COMPILER MATCHES gfortran|f95)
    set (FORTRAN_LIB gfortran)
  else (CMAKE_Fortran_COMPILER MATCHES gfortran|f95)
    set (FORTRAN_LIB g2c)
  endif (CMAKE_Fortran_COMPILER MATCHES gfortran|f95)

  set (GCC_LIBRARIES
    ${FORTRAN_LIB}
  )

  find_library (THREAD_LIB pthread pthreads)
  set          (SOCKET_LIB)
endif (MSVC)

# Under Linux, we want to link with librt in order to use finer grained timers.
if (CMAKE_SYSTEM_NAME MATCHES Linux)
  find_library (RT_LIB rt)
else (CMAKE_SYSTEM_NAME MATCHES Linux)
  set          (RT_LIB)
endif (CMAKE_SYSTEM_NAME MATCHES Linux)

# The LAPACK package finder that comes with cmake seems to be broken, even when
# Fortran is available.
find_package (LAPACKnofortran)
find_package (TIFF)
find_package (GTIF)
find_package (JPEG)
find_package (PNG)
find_package (Freetype)
find_package (X11)
find_package (FFMPEG)

if (LAPACK_FOUND)
  add_definitions (-DHAVE_LAPACK)
endif (LAPACK_FOUND)

if (BLAS_FOUND)
  add_definitions (-DHAVE_BLAS)
endif (BLAS_FOUND)

if (TIFF_FOUND)
  add_definitions (-DHAVE_TIFF)
endif (TIFF_FOUND)

if (GTIF_FOUND)
  add_definitions (-DHAVE_GTIF)
endif (GTIF_FOUND)

if (JPEG_FOUND)
  add_definitions (-DHAVE_JPEG)
endif (JPEG_FOUND)

if (PNG_FOUND)
  add_definitions (-DHAVE_PNG)
endif (PNG_FOUND)

if (Freetype_FOUND)
  add_definitions (-DHAVE_FREETYPE)
endif (Freetype_FOUND)

if (X11_FOUND)
  add_definitions (-DHAVE_X11)
endif (X11_FOUND)

if (FFMPEG_FOUND)
  add_definitions (-DHAVE_FFMPEG)
endif (FFMPEG_FOUND)

include_directories (include)


# Set destination for targets -------------------------------------------------

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if (MSVC)
    # Install to the root of the source directory.  This compensates for
    # the VS generator's (apparent) lack of ability to cleanly place just
    # the target file in the desired output directory.  Try to discover a
    # cleaner way to do this so we don't overload the install process.
    set (CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR} CACHE PATH "Where to install" FORCE)
  else (MSVC)
    # An arbitrary install location consistent with the use of the Encap package manager on *nix systems
    set (CMAKE_INSTALL_PREFIX /usr/encap/fl CACHE PATH "Where to install" FORCE)
  endif (MSVC)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# See above comment on MSVC install prefix.  The VS generator places some
# intermediate files in the output directory, which is ugly.
if (NOT MSVC)
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib64)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib64)
  else (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
  endif (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (DIRECTORY script/
    DESTINATION bin
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )
endif (NOT MSVC)

install (DIRECTORY include/fl DESTINATION include)


# Add all targets -------------------------------------------------------------

add_subdirectory (src/base)
add_subdirectory (src/net)
add_subdirectory (src/numeric)
add_subdirectory (src/image)
if (NOT MSVC)
  add_subdirectory (src/X)
  add_subdirectory (src/util)
endif (NOT MSVC)