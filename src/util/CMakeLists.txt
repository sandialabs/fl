# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_executable (addm addm.cc)
install (TARGETS addm RUNTIME DESTINATION bin)

add_executable (convertall convertall.cc)
install (TARGETS convertall RUNTIME DESTINATION bin)

add_executable (epsall epsall.cc)
install (TARGETS epsall RUNTIME DESTINATION bin)

add_executable (stripm stripm.cc)
install (TARGETS stripm RUNTIME DESTINATION bin)

add_executable (striputf8 striputf8.cc)
install (TARGETS striputf8 RUNTIME DESTINATION bin)

if (LAPACK_FOUND)
  add_executable (invert invert.cc)
  target_link_libraries (invert
    flNumeric
    flNet
    flBase
    ${FL_LIBRARIES}
  )
  install (TARGETS invert RUNTIME DESTINATION bin)

  if (FFMPEG_FOUND)
    add_executable (psnr psnr.cc)
    target_link_libraries (psnr
      flImage
      flNumeric
      flNet
      flBase
      ${FL_LIBRARIES}
    )
    install (TARGETS psnr RUNTIME DESTINATION bin)

    if (JPEG_FOUND)
      add_executable (thumb thumb.cc)
      target_link_libraries (thumb
        flImage
        flNumeric
        flNet
        flBase
        ${FL_LIBRARIES}
      )
      install (TARGETS thumb RUNTIME DESTINATION bin)
    endif (JPEG_FOUND)

    if (X11_FOUND  AND  JPEG_FOUND)
      add_executable (vread vread.cc)
      target_link_libraries (vread
        flX
        flImage
        flNumeric
        flNet
        flBase
        ${FL_LIBRARIES}
      )
      install (TARGETS vread RUNTIME DESTINATION bin)
    endif (X11_FOUND  AND  JPEG_FOUND)
  endif (FFMPEG_FOUND)

  if (X11_FOUND)
    add_executable (xv xv.cc)
    target_link_libraries (xv
      flX
      flImage
      flNumeric
      flNet
      flBase
      ${FL_LIBRARIES}
    )
    install (TARGETS xv RUNTIME DESTINATION bin)
  endif (X11_FOUND)
endif (LAPACK_FOUND)
