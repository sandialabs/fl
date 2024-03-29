# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


if (Freetype_FOUND)
  include_directories (${FREETYPE_INCLUDE_DIRS})
endif (Freetype_FOUND)

set (maybeImage)

if (TIFF_FOUND)
  set (maybeImage ${maybeImage} ImageFileFormatTIFF.cc)
endif (TIFF_FOUND)

if (JPEG_FOUND)
  set (maybeImage ${maybeImage} ImageFileFormatJPEG.cc)
endif (JPEG_FOUND)

if (PNG_FOUND)
  set (maybeImage ${maybeImage} ImageFileFormatPNG.cc)
endif (PNG_FOUND)

if (FFMPEG_FOUND)
  # Should rearrange code so that Video.cc and video.h can be built without FFMPEG present.
  set (maybeVideo
    Video.cc
    VideoFileFormatFFMPEG.cc
  )
endif (FFMPEG_FOUND)

if (X11_FOUND  OR  MSVC)
  set (maybeX flX)
endif (X11_FOUND  OR  MSVC)

if (LAPACK_FOUND)
  set (maybeMatch Match.cc)

  set (maybeDescriptors
    DescriptorFilters.cc
    DescriptorFiltersTexton.cc
  )
endif (LAPACK_FOUND)

add_library (flImage
  # Base Image
  ../../include/fl/image.h
  ../../include/fl/point.h
  Image.cc
  Pixel.cc
  PixelFormat.cc
  PixelBuffer.cc
  Point.cc
  Rectangle.cc

  # Image cache
  ../../include/fl/imagecache.h
  ImageCache.cc
  EntryFiniteDifference.cc
  EntryDOG.cc

  # Image file formats
  ImageFileFormat.cc
  ImageFileFormatBMP.cc
  ImageFileFormatPGM.cc
  ImageFileFormatRRIF.cc
  ImageFileFormatEPS.cc
  ImageFileFormatMatlab.cc
  ImageFileFormatNITF.cc
  ${maybeImage}

  # Canvas
  ../../include/fl/canvas.h
  ../../include/fl/color.h
  Canvas.cc
  CanvasImage.cc
  CanvasPS.cc

  # Filters and convolutions
  ../../include/fl/convolve.h
  Convolution2D.cc
  Gaussian2D.cc
  DifferenceOfGaussians.cc
  GaussianDerivativeFirst.cc
  GaussianDerivativeSecond.cc
  GaussianDerivativeThird.cc
  Laplacian.cc
  Convolution1D.cc
  Gaussian1D.cc
  GaussianDerivative1D.cc
  GaussianDerivativeSecond1D.cc
  ConvolutionRecursive.cc
  FilterHarris.cc
  FilterHarrisEigen.cc
  FilterHessian.cc
  FiniteDifference.cc
  NonMaxSuppress.cc
  Median.cc
  IntensityStatistics.cc
  IntensityHistogram.cc
  Normalize.cc
  AbsoluteValue.cc
  Rescale.cc
  Rotate90.cc
  Rotate180.cc
  Transform.cc
  TransformGauss.cc
  TransformNeighbor.cc
  TransformEagle.cc
  Decimate.cc
  DoubleSize.cc
  BlurDecimate.cc
  Zoom.cc
  ClearAlpha.cc

  # Descriptors
  ../../include/fl/descriptor.h
  Comparison.cc
  Descriptor.cc
  DescriptorColorHistogram2D.cc
  DescriptorColorHistogram3D.cc
  DescriptorCombo.cc
  DescriptorContrast.cc
  DescriptorLBP.cc
  DescriptorOrientation.cc
  DescriptorOrientationHistogram.cc
  DescriptorPatch.cc
  DescriptorScale.cc
  DescriptorSchmidScale.cc
  DescriptorSchmid.cc
  DescriptorSIFT.cc
  DescriptorSpatialPyramid.cc
  DescriptorSpin.cc
  DescriptorTextonScale.cc
  ${maybeDescriptors}

  # Interest operators
  ../../include/fl/interest.h
  Interest.cc
  InterestDOG.cc
  InterestHarris.cc
  InterestHarrisLaplacian.cc
  InterestHessian.cc
  InterestLaplacian.cc
  InterestMSER.cc

  # Matching framework
  ../../include/fl/match.h
  ${maybeMatch}

  # Video
  ../../include/fl/video.h
  ../../include/fl/track.h
  KLT.cc
  ${maybeVideo}
)
target_link_libraries (flImage
  flNumeric
  flBase
  ${FL_LIBRARIES}
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flImage
    RUNTIME DESTINATION bin64
    LIBRARY DESTINATION lib64
    ARCHIVE DESTINATION lib64
  )
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flImage
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

add_executable (testImage test.cc)
target_link_libraries (testImage
  ${maybeX}
  flImage
  flNumeric
  flNet
  flBase
  ${FL_LIBRARIES}
)
