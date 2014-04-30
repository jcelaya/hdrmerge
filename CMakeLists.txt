cmake_minimum_required(VERSION 2.6)

project(hdrmerge)

set (CMAKE_CXX_FLAGS -std=c++11)

# Required and optional packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
# Qt4
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

# Boost Test
find_package(Boost 1.46 COMPONENTS unit_test_framework)

# OpenMP
find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# LibRaw
find_package(LibRaw REQUIRED)

# Exiv2
find_package(Exiv2 REQUIRED)

# Build dngsdk and xmpsdk
if(WIN32) # Set Windows rules.
    add_definitions(-DqWinOS=1 -DqMacOS=0 -DWIN_ENV=1)
else(WIN32) # Set Linux & co rules.
    add_definitions(-DqWinOS=0 -DqMacOS=0 -DqLinux=1 -DqAndroid=1 -DUNIX_ENV=1 -DqDNGUseStdInt=1 -DqDNGThreadSafe=1)
endif(WIN32)
add_definitions(-DqDNGDebug=0 -DqDNGValidateTarget=0 -DqDNGUseLibJPEG=0 -DqDNGXMPDocOps=0)

# Check processor endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(NOT IS_BIG_ENDIAN)
    add_definitions(-DqDNGLittleEndian=1)
endif(NOT IS_BIG_ENDIAN)

include_directories(third_party/dng_sdk third_party/xmp_sdk/public/include)
add_subdirectory(third_party)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# The version number.
set (HDRMERGE_VERSION_MAJOR 0)
set (HDRMERGE_VERSION_MINOR 3)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")

# Sources and headers
set(hdrmerge_sources
    gui.cpp MainWindow.cpp PreviewWidget.cpp DraggableScrollArea.cpp
    Image.cpp ImageStack.cpp Bitmap.cpp MetaData.cpp MergeMap.cpp DngWriter.cpp
    camxyz.cpp exiv2dngstreamio.cpp exiv2meta.cpp)
set(hdrmerge_qobject_headers MainWindow.hpp PreviewWidget.hpp
    DraggableScrollArea.hpp AboutDialog.hpp)
set(hdrmerge_translations hdrmerge_en.ts hdrmerge_es.ts)

# Libs
set(hdrmerge_libs ${QT_LIBRARIES} ${EXIV2_LIBRARY} ${LibRaw_LIBRARIES} dngsdk ${XMP_LIBRARIES} pthread dl)

# Qt4 intermediate files
QT4_WRAP_CPP(hdrmerge_moc ${hdrmerge_qobject_headers})
QT4_ADD_TRANSLATION(hdrmerge_qm ${hdrmerge_translations})
# Generate the XML version of hdrmerge_qm
foreach(file ${hdrmerge_qm})
    set(HDRMERGE_QM_XML "${HDRMERGE_QM_XML}
        <file>${file}</file>")
endforeach(file)
configure_file (
    "${PROJECT_SOURCE_DIR}/translations.qrc.in"
    "${PROJECT_BINARY_DIR}/translations.qrc"
)
QT4_ADD_RESOURCES(hdrmerge_rsrc ${PROJECT_BINARY_DIR}/translations.qrc resources.qrc)

add_library(hdrmerge-objects OBJECT ${hdrmerge_sources} ${hdrmerge_moc} ${hdrmerge_rsrc})
add_executable(hdrmerge main.cpp $<TARGET_OBJECTS:hdrmerge-objects>)
target_link_libraries(hdrmerge ${hdrmerge_libs})

install(TARGETS hdrmerge RUNTIME DESTINATION bin)

if (Boost_FOUND)
    add_subdirectory(test)
endif (Boost_FOUND)
