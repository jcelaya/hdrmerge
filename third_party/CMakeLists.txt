FIND_LIBRARY(XMPCORE_LIBRARIES NAMES XMPCore PATHS xmp_sdk/public/libraries)
FIND_LIBRARY(XMPFILES_LIBRARIES NAMES XMPFiles PATHS xmp_sdk/public/libraries)
IF(XMPCORE_LIBRARIES AND XMPFILES_LIBRARIES)
	SET(XMP_LIBRARIES ${XMPCORE_LIBRARIES} ${XMPFILES_LIBRARIES} PARENT_SCOPE)
ELSE()
	MESSAGE(FATAL_ERROR "XMP libs not found!")
ENDIF()

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/xmp_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/xmp_sdk/public/include
)

# Set platteforms env.
IF(WIN32)
    ADD_DEFINITIONS(
                    # Set Windows rules.
                    -DqWinOS=1
                    -DqMacOS=0
                    -DWIN_ENV=1
                   )
ELSE(WIN32)
    ADD_DEFINITIONS(
                    # Set MACOS-X, Linux & co rules.
                    -DqWinOS=0
                    -DqMacOS=0
                    -DqLinux=1
                    -DqAndroid=1
                    -DUNIX_ENV=1

                    # Must be set to 1, else do not compile under Linux.
                    -DqDNGUseStdInt=1
                   )
ENDIF(WIN32)

# Check processor endianness
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
IF(NOT IS_BIG_ENDIAN)
    ADD_DEFINITIONS(-DqDNGLittleEndian=1)
ENDIF(NOT IS_BIG_ENDIAN)

# Thread safe support under Mac and Linux using pthread library
IF(NOT WIN32)
    ADD_DEFINITIONS(-DqDNGThreadSafe=1)
ENDIF(NOT WIN32)

# Level of debug info in the console.
ADD_DEFINITIONS(-DqDNGDebug=0)
ADD_DEFINITIONS(-DqDNGValidateTarget=1)
ADD_DEFINITIONS(-DqDNGUseLibJPEG=0)

SET( LIBDNGSDK_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_1d_function.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_1d_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_abort_sniffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_area_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_bad_pixels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_bottlenecks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_camera_profile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_color_space.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_color_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_date_time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_exif.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_file_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_filter_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_fingerprint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_gain_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_globals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_hue_sat_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_ifd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_image_writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_iptc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_jpeg_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_lens_correction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_linearization_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_lossless_jpeg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_memory_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_misc_opcodes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_mosaic_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_negative.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_opcode_list.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_opcodes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_orientation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_parse_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_pixel_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_preview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_pthread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_rational.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_read_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_rect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_ref_counted_block.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_reference.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_resample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_shared.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_simple_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_spline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_string.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_string_list.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_tag_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_temperature.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_tile_iterator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_tone_curve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_xmp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_xmp_sdk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_xy_coord.cpp
   )

ADD_LIBRARY( dngsdk ${LIBDNGSDK_SRCS} )
#ADD_EXECUTABLE( dngvalidate ${CMAKE_CURRENT_SOURCE_DIR}/dng_sdk/dng_validate.cpp )
#TARGET_LINK_LIBRARIES( dngvalidate dngsdk pthread dl)

