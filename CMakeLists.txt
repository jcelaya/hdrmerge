cmake_minimum_required(VERSION 2.6)

project(hdrmerge)

set (CMAKE_CXX_FLAGS -std=c++11)

# Required and optional packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
# Qt4
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

# libtiff
find_package(TIFF REQUIRED)
include(${TIFF_INCLUDE_DIR})

# Boost Test
find_package(Boost 1.46 COMPONENTS unit_test_framework)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# The version number.
set (HDRMERGE_VERSION_MAJOR 0)
set (HDRMERGE_VERSION_MINOR 2)

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
    gui.cpp MainWindow.cpp RenderThread.cpp ImageControl.cpp PreviewWidget.cpp DraggableScrollArea.cpp
    Exposure.cpp Image.cpp ImageStack.cpp Bitmap.cpp MetaData.cpp Postprocess.cpp MergeMap.cpp)
set(hdrmerge_qobject_headers MainWindow.hpp RenderThread.hpp ImageControl.hpp PreviewWidget.hpp
    DraggableScrollArea.hpp AboutDialog.hpp SaveProgress.hpp)
set(hdrmerge_translations hdrmerge_en.ts hdrmerge_es.ts)

# Link with qt4, libtiff and libpfs-1.2
# TODO: find them at build time
set(hdrmerge_libs ${QT_LIBRARIES} ${TIFF_LIBRARIES} pfs-1.2 raw)

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
