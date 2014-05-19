cmake_minimum_required(VERSION 2.8)

project(hdrmerge)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Required and optional packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
# Qt4
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
find_package(LibRaw REQUIRED)
find_package(Exiv2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost 1.46 COMPONENTS unit_test_framework)
include_directories(${LibRaw_INCLUDE_DIR} ${EXIV2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
if (WIN32)
	set(QT_LIBRARIES ${QT_QTCORE_LIBRARY_RELEASE} ${QT_QTGUI_LIBRARY_RELEASE})
else (WIN32)
	set(WIN32_LIBS "")
endif (WIN32)

# OpenMP
find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# The version number.
set (HDRMERGE_VERSION_MAJOR 0)
set (HDRMERGE_VERSION_MINOR 4)

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
    Launcher.cpp MainWindow.cpp PreviewWidget.cpp DraggableScrollArea.cpp DngPropertiesDialog.cpp LoadOptionsDialog.cpp
    Image.cpp ImageStack.cpp Bitmap.cpp MetaData.cpp EditableMask.cpp DngFloatWriter.cpp Renderer.cpp
    camxyz.cpp TiffDirectory.cpp)
set(hdrmerge_qobject_headers MainWindow.hpp PreviewWidget.hpp LoadOptionsDialog.hpp
    DraggableScrollArea.hpp AboutDialog.hpp DngPropertiesDialog.hpp)
set(hdrmerge_translations hdrmerge_es.ts)

# Libs
set(hdrmerge_libs ${QT_LIBRARIES} ${EXIV2_LIBRARY} ${LibRaw_LIBRARIES} ${ZLIB_LIBRARIES} ${WIN32_LIBS})

# Qt4 intermediate files
QT4_WRAP_CPP(hdrmerge_moc ${hdrmerge_qobject_headers})
QT4_ADD_TRANSLATION(hdrmerge_qm ${hdrmerge_translations})
# Generate the XML version of hdrmerge_qm
foreach(file ${hdrmerge_qm})
    get_filename_component(basename ${file} NAME)
    set(HDRMERGE_QM_XML "${HDRMERGE_QM_XML}
        <file>${basename}</file>")
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
