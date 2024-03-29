# - Find FL and its supporting libraries
# This module defines
#   FL_FOUND         If false, do not try to use FL.
#   FL_INCLUDE_DIRS  Directories containing the include files
#   FL_LIBRARIES     Full path to libraries to link against

# Author: Fred Rothganger
# Copyright 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


set (FL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/.. CACHE PATH "FL installation directory. Should contain include and lib subdirs.")

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

if    (CMAKE_CL_64)
  set_property (GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
endif (CMAKE_CL_64)

set (CMAKE_PREFIX_PATH ${FL_ROOT_DIR})

find_path (FL_INCLUDE_DIRS fl/image.h)

find_library (FL_X       flX)
find_library (FL_IMAGE   flImage)
find_library (FL_NUMERIC flNumeric)
find_library (FL_BASE    flBase)
find_library (FL_NET     flNet)

set (FL_LIBRARIES)
if (NOT FL_BUILD)
  if    (FL_X)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_X})
  endif (FL_X)

  if    (FL_IMAGE)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_IMAGE})
  endif (FL_IMAGE)

  if    (FL_NUMERIC)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_NUMERIC})
  endif (FL_NUMERIC)

  if    (FL_BASE)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_BASE})
  endif (FL_BASE)

  if    (FL_NET)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_NET})
  endif (FL_NET)
endif (NOT FL_BUILD)


# handle the QUIETLY and REQUIRED arguments
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FL  DEFAULT_MSG  FL_LIBRARIES FL_INCLUDE_DIRS)


option (FL_USE_LAPACK   "Turn OFF to suppress LAPACK"   ON)
option (FL_USE_BLAS     "Turn OFF to suppress BLAS"     ON)
option (FL_USE_FFTW     "Turn OFF to suppress FFTW"     ON)
option (FL_USE_TIFF     "Turn OFF to suppress TIFF"     ON)
option (FL_USE_GTIF     "Turn OFF to suppress GTIF"     ON)
option (FL_USE_JPEG     "Turn OFF to suppress JPEG"     ON)
option (FL_USE_PNG      "Turn OFF to suppress PNG"      ON)
option (FL_USE_Freetype "Turn OFF to suppress Freetype" ON)
option (FL_USE_X11      "Turn OFF to suppress X11"      ON)
option (FL_USE_OpenGL   "Turn OFF to suppress OpenGL"   ON)
option (FL_USE_FFMPEG   "Turn OFF to suppress FFMPEG"   ON)

# Prioritize /usr ahead of / when searching for libraries.  Work-around for
# interaction between encap symbolic links and directory shadowing under Cygwin.
set (CMAKE_PREFIX_PATH /usr $ENV{MKL_HOME})

set (CMAKE_MODULE_PATH ${FL_ROOT_DIR}/cmake)
if (MSVC)
  set (CMAKE_INCLUDE_PATH ${FL_ROOT_DIR}/mswin/include)
  if    (CMAKE_CL_64)
    set (CMAKE_LIBRARY_PATH ${FL_ROOT_DIR}/mswin/lib64)
  else  (CMAKE_CL_64)
    set (CMAKE_LIBRARY_PATH ${FL_ROOT_DIR}/mswin/lib)
  endif (CMAKE_CL_64)

  # Add compiler support libraries.
  find_library (FORTRAN_LIB  fortran)
  if    (FORTRAN_LIB)
    set (GCC_LIBRARIES ${GCC_LIBRARIES} ${FORTRAN_LIB})
  endif (FORTRAN_LIB)

  set (SOCKET_LIBRARIES ws2_32)

  # Some includes are available by default under Unix but are not available
  # under MSVC, so add the supplemental include directory preemtively.
  set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FL_ROOT_DIR}/mswin/include)

  # Avoid linkage conflicts caused by using mswin libraries.
  if    (NOT CMAKE_EXE_LINKER_FLAGS MATCHES libcmt)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt;libcmtd" CACHE STRING "linker flags" FORCE)
  endif (NOT CMAKE_EXE_LINKER_FLAGS MATCHES libcmt)
  if    (NOT CMAKE_EXE_LINKER_FLAGS MATCHES NOREF)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:NOREF" CACHE STRING "linker flags" FORCE)
  endif (NOT CMAKE_EXE_LINKER_FLAGS MATCHES NOREF)

  if    (NOT CMAKE_SHARED_LINKER_FLAGS MATCHES libcmt)
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libcmt;libcmtd" CACHE STRING "linker flags" FORCE)
  endif (NOT CMAKE_SHARED_LINKER_FLAGS MATCHES libcmt)
  if    (NOT CMAKE_SHARED_LINKER_FLAGS MATCHES NOREF)
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:NOREF" CACHE STRING "linker flags" FORCE)
  endif (NOT CMAKE_SHARED_LINKER_FLAGS MATCHES NOREF)

  # Use multiple-processors to compile c/c++ code
  if    (NOT CMAKE_CXX_FLAGS MATCHES /MP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" CACHE STRING "C++ compiler flags" FORCE)
    set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /MP" CACHE STRING "C compiler flags"   FORCE)
  endif (NOT CMAKE_CXX_FLAGS MATCHES /MP)
endif (MSVC)

# Under Linux, we want to link with librt in order to use finer grained timers.
if    (CMAKE_SYSTEM_NAME MATCHES Linux)
  find_library (RT_LIBRARIES rt)
else  (CMAKE_SYSTEM_NAME MATCHES Linux)
  set          (RT_LIBRARIES)
endif (CMAKE_SYSTEM_NAME MATCHES Linux)

if    (FL_USE_BLAS)
  find_package (BLAS)

  if    (FL_USE_LAPACK)
    find_package (LAPACK)
    if    (LAPACK_FOUND)
      add_definitions (-DHAVE_LAPACK)
      set (FL_LIBRARIES ${FL_LIBRARIES} ${LAPACK_LIBRARIES})
    endif (LAPACK_FOUND)
  endif (FL_USE_LAPACK)

  if    (BLAS_FOUND)
    add_definitions (-DHAVE_BLAS)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${BLAS_LIBRARIES})
  endif (BLAS_FOUND)
endif (FL_USE_BLAS)

if    (FL_USE_FFTW)
  find_package (FFTW)
  if    (FFTW_FOUND)
    add_definitions (-DHAVE_FFTW)
    # For the most part, include paths are standard and don't need to be
    # added.  However, on HPC systems FFTW gets special treatment so needs
    # special pathing.
    set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS})
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FFTW_LIBRARIES})
  endif (FFTW_FOUND)
endif (FL_USE_FFTW)

if    (FL_USE_TIFF)
  find_package (TIFF)
  if    (TIFF_FOUND)
    add_definitions (-DHAVE_TIFF)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${TIFF_LIBRARIES})
  endif (TIFF_FOUND)
endif (FL_USE_TIFF)

if    (FL_USE_GTIF)
  find_package (GTIF)
  if    (GTIF_FOUND)
    add_definitions (-DHAVE_GTIF)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${GTIF_LIBRARIES})
  endif (GTIF_FOUND)
endif (FL_USE_GTIF)

if    (FL_USE_JPEG)
  find_package (JPEG)
  if    (JPEG_FOUND)
    add_definitions (-DHAVE_JPEG)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${JPEG_LIBRARIES})
  endif (JPEG_FOUND)
endif (FL_USE_JPEG)

if    (FL_USE_PNG)
  find_package (PNG)
  if    (PNG_FOUND)
    add_definitions (-DHAVE_PNG)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${PNG_LIBRARIES})
  endif (PNG_FOUND)
endif (FL_USE_PNG)

if    (FL_USE_Freetype)
  find_package (Freetype)
  if    (FREETYPE_FOUND)
    add_definitions (-DHAVE_FREETYPE)
    set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FREETYPE_LIBRARIES})
  endif (FREETYPE_FOUND)
endif (FL_USE_Freetype)

if    (FL_USE_X11)
  find_package (X11)
  if    (X11_FOUND)
    add_definitions (-DHAVE_X11)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${X11_LIBRARIES})
  endif (X11_FOUND)
endif (FL_USE_X11)

if    (FL_USE_OpenGL)
  set (OpenGL_GL_PREFERENCE GLVND)  # allows compatibility with pre-3.10 versions of CMake
  find_package (OpenGL)
endif (FL_USE_OpenGL)

if    (FL_USE_FFMPEG)
  find_package (FFMPEG)
  if    (FFMPEG_FOUND)
    add_definitions (-DHAVE_FFMPEG)
    set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FFMPEG_LIBRARIES})
  endif (FFMPEG_FOUND)
endif (FL_USE_FFMPEG)

if    (CMAKE_USE_PTHREADS_INIT)
  add_definitions (-DHAVE_PTHREAD)
endif (CMAKE_USE_PTHREADS_INIT)

set (FL_LIBRARIES ${FL_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${GCC_LIBRARIES}
  ${RT_LIBRARIES}
  ${SOCKET_LIBRARIES}
)

mark_as_advanced (
  FL_INCLUDE_DIRS
  FL_LIBRARIES
  FL_X
  FL_IMAGE
  FL_NUMERIC
  FL_BASE
  FL_NET
  RT_LIBRARIES
  CMAKE_THREAD_LIBS_INIT
)
