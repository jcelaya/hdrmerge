cmake_minimum_required(VERSION 2.8)

project(hdrmerge)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set (PLATFORM "")

# Required and optional packages
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
find_package(LibRaw REQUIRED)
find_package(Exiv2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost 1.46 COMPONENTS unit_test_framework)
include_directories("${LibRaw_INCLUDE_DIR}" "${EXIV2_INCLUDE_DIR}" "${ZLIB_INCLUDE_DIRS}")

# OpenMP
find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# The version number.
set (HDRMERGE_VERSION_MAJOR 0)
set (HDRMERGE_VERSION_MINOR 4)
set (HDRMERGE_VERSION_REV 5)
set (HDRMERGE_VERSION ${HDRMERGE_VERSION_MAJOR}.${HDRMERGE_VERSION_MINOR}.${HDRMERGE_VERSION_REV})

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")

set (WIN_RES_FILE "")
if (WIN32)
    # This is required by libraw.h
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32")
    # TODO: right now, we prefer a console application
    #set(PLATFORM WIN32)
    configure_file (
        "${PROJECT_SOURCE_DIR}/winres.rc"
        "${PROJECT_BINARY_DIR}/winres.rc"
    )
    set(WIN_RES_FILE "${PROJECT_BINARY_DIR}/winres.res")
    add_custom_command(OUTPUT "${WIN_RES_FILE}"
        COMMAND "${CMAKE_RC_COMPILER}" "${PROJECT_BINARY_DIR}/winres.rc" -O coff -o "${WIN_RES_FILE}"
        MAIN_DEPENDENCY "${PROJECT_BINARY_DIR}/winres.rc"
        DEPENDS "images/icon.ico")
endif (WIN32)

# Sources and headers
set(hdrmerge_sources
    Launcher.cpp MainWindow.cpp PreviewWidget.cpp DraggableScrollArea.cpp DngPropertiesDialog.cpp LoadOptionsDialog.cpp
    Image.cpp ImageStack.cpp Bitmap.cpp RawParameters.cpp EditableMask.cpp DngFloatWriter.cpp
    TiffDirectory.cpp AboutDialog.cpp BoxBlur.cpp ExifTransfer.cpp ImageIO.cpp)
set(hdrmerge_qobject_headers MainWindow.hpp PreviewWidget.hpp LoadOptionsDialog.hpp
    DraggableScrollArea.hpp DngPropertiesDialog.hpp AboutDialog.hpp)
set(hdrmerge_translations hdrmerge_es.ts)

# Libs
set(hdrmerge_libs "${QT_LIBRARIES}" "${EXIV2_LIBRARY}" "${LibRaw_r_LIBRARIES}" "${ZLIB_LIBRARIES}")

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
QT4_ADD_RESOURCES(hdrmerge_rsrc "${PROJECT_BINARY_DIR}/translations.qrc" resources.qrc)

add_library(hdrmerge-objects OBJECT ${hdrmerge_sources} ${hdrmerge_moc} ${hdrmerge_rsrc})
add_executable(hdrmerge ${PLATFORM} main.cpp $<TARGET_OBJECTS:hdrmerge-objects> "${WIN_RES_FILE}")
target_link_libraries(hdrmerge ${hdrmerge_libs})

if (WIN32)
    # Create the installer with makensis
    find_program(MAKENSIS_EXECUTABLE makensis)
    if (MAKENSIS_EXECUTABLE)
        get_filename_component(LIBEXIV2_PATH "${EXIV2_LIBRARY}" PATH)
        find_library(ZLIB_DLL zlib1.dll)
        find_library(EXPAT_DLL libexpat-1.dll "${LIBEXIV2_PATH}")
        find_library(ICONV_DLL libiconv-2.dll "${LIBEXIV2_PATH}")
        find_library(QT_JPEG_DLL qjpeg4.dll "${QT_ROOT}/plugins/imageformats")
        find_library(QT_CORE_DLL QtCore4.dll "${QT_ROOT}/lib")
        find_library(QT_GUI_DLL QtGui4.dll "${QT_ROOT}/lib")
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libgcc_s_sjlj-1.dll OUTPUT_VARIABLE MINGW_SJLJ OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libgomp-1.dll OUTPUT_VARIABLE MINGW_GOMP OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libstdc++-6.dll OUTPUT_VARIABLE MINGW_STDCPP OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libwinpthread-1.dll OUTPUT_VARIABLE MINGW_WINPT OUTPUT_STRIP_TRAILING_WHITESPACE)

        set (SETUP_PROG hdrmerge-setup${WIN_ARCH}-${HDRMERGE_VERSION}.exe)
        configure_file (
            "${PROJECT_SOURCE_DIR}/setup.nsi"
            "${PROJECT_BINARY_DIR}/setup.nsi" @ONLY
        )
        add_custom_command(OUTPUT ${SETUP_PROG}
            COMMAND "${MAKENSIS_EXECUTABLE}" setup.nsi#-V2 setup.nsi
            MAIN_DEPENDENCY setup.nsi
            DEPENDS "${PROJECT_BINARY_DIR}/hdrmerge.exe"
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
        add_custom_target(hdrmerge-setup ALL DEPENDS "${PROJECT_BINARY_DIR}/${SETUP_PROG}")
    endif (MAKENSIS_EXECUTABLE)
else (WIN32)
    install(TARGETS hdrmerge RUNTIME DESTINATION bin)
    find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
    find_program(XDG-ICON-RESOURCE_EXECUTABLE xdg-icon-resource)
    if (XDG-DESKTOP-MENU_EXECUTABLE AND XDG-ICON-RESOURCE_EXECUTABLE)
        configure_file (
            "${PROJECT_SOURCE_DIR}/hdrmerge.desktop"
            "${PROJECT_BINARY_DIR}/hdrmerge.desktop"
        )
        install(CODE "
            execute_process(COMMAND \"${XDG-ICON-RESOURCE_EXECUTABLE}\" install --novendor
                --size 128 \"${PROJECT_SOURCE_DIR}/images/icon.png\" hdrmerge-icon)
            execute_process(COMMAND \"${XDG-DESKTOP-MENU_EXECUTABLE}\" install --novendor
                \"${PROJECT_BINARY_DIR}/hdrmerge.desktop\")"
        )
    endif (XDG-DESKTOP-MENU_EXECUTABLE AND XDG-ICON-RESOURCE_EXECUTABLE)
endif (WIN32)

if (Boost_FOUND)
    add_subdirectory(test)
endif (Boost_FOUND)

