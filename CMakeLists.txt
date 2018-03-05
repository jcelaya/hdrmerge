cmake_minimum_required(VERSION 2.8)

project(hdrmerge)

if(WIN32)
    if(NOT(ALGLIB_ROOT))
        message(FATAL_ERROR "When building on Windows, you need to download 'alglib' source and define its root directory in ALGLIB_ROOT !")
    else()
        message(STATUS "ALGLIB_ROOT = ${ALGLIB_ROOT}")
    endif()
endif()

# Complain about anything, except ignored OpenMP pragma in Debug
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-unknown-pragmas")
set(PLATFORM "")

# Required and optional packages
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
find_package(LibRaw REQUIRED)
find_package(Exiv2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost 1.46 COMPONENTS unit_test_framework)
find_package(OpenMP)

if(WIN32)
    set(ALGLIB_INCLUDES ${ALGLIB_ROOT}/src)
else()
    find_package(Alglib REQUIRED)
endif()

include_directories(
    "${LibRaw_INCLUDE_DIR}"
    "${EXIV2_INCLUDE_DIR}"
    "${ZLIB_INCLUDE_DIRS}"
    "${ALGLIB_INCLUDES}"
)

if(NOT(CMAKE_BUILD_TYPE))
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(OPENMP_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
    if(CMAKE_COMPILER_IS_GNUCC)
        set(STRIP -s)
    endif()
endif()

# The version number.
set(HDRMERGE_VERSION_MAJOR 0)
set(HDRMERGE_VERSION_MINOR 5)
set(HDRMERGE_VERSION_REV 0)
set(HDRMERGE_VERSION ${HDRMERGE_VERSION_MAJOR}.${HDRMERGE_VERSION_MINOR}.${HDRMERGE_VERSION_REV})

# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/src/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")

set(PLATFORM_SOURCES "")
if(WIN32)
    # This is required by libraw.h
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32 -DLIBRAW_NODLL")
    set(PLATFORM WIN32)
    configure_file(
        "${PROJECT_SOURCE_DIR}/data/winres.rc"
        "${PROJECT_BINARY_DIR}/winres.rc"
    )
    set(PLATFORM_SOURCES "${PROJECT_BINARY_DIR}/winres.rc")
elseif(APPLE)
    set(PLATFORM MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_ICON_FILE "icon")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.hdrmerge")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "HDRMerge for macOS v${HDRMERGE_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_NAME "HDRMerge")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${HDRMERGE_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${HDRMERGE_VERSION}")
    set(MACOSX_BUNDLE_INFO_COPYRIGHT "Copyright 2018 Javier Celaya")
    set(PLATFORM_SOURCES "images/icon.icns")
endif()

# Sources and headers
set(hdrmerge_sources
    src/Image.cpp
    src/ImageStack.cpp
    src/Bitmap.cpp
    src/RawParameters.cpp
    src/EditableMask.cpp
    src/DngFloatWriter.cpp
    src/TiffDirectory.cpp
    src/BoxBlur.cpp
    src/ExifTransfer.cpp
    src/ImageIO.cpp
)

set(hdrmerge_gui_sources
    src/AboutDialog.cpp
    src/MainWindow.cpp
    src/PreviewWidget.cpp
    src/DraggableScrollArea.cpp
    src/DngPropertiesDialog.cpp
    src/LoadOptionsDialog.cpp
)

set(hdrmerge_qobject_headers
    src/MainWindow.hpp
    src/PreviewWidget.hpp
    src/LoadOptionsDialog.hpp
    src/DraggableScrollArea.hpp
    src/DngPropertiesDialog.hpp
    src/AboutDialog.hpp
)

set(hdrmerge_translations
    data/hdrmerge_es.ts
)

if(WIN32)
    set(alglib_sources
        ${ALGLIB_ROOT}/src/alglibinternal.cpp
        ${ALGLIB_ROOT}/src/alglibmisc.cpp
        ${ALGLIB_ROOT}/src/ap.cpp
        ${ALGLIB_ROOT}/src/dataanalysis.cpp
        ${ALGLIB_ROOT}/src/diffequations.cpp
        ${ALGLIB_ROOT}/src/fasttransforms.cpp
        ${ALGLIB_ROOT}/src/integration.cpp
        ${ALGLIB_ROOT}/src/interpolation.cpp
        ${ALGLIB_ROOT}/src/linalg.cpp
        ${ALGLIB_ROOT}/src/optimization.cpp
        ${ALGLIB_ROOT}/src/solvers.cpp
        ${ALGLIB_ROOT}/src/specialfunctions.cpp
        ${ALGLIB_ROOT}/src/statistics.cpp
    )
endif()

# Libs
set(hdrmerge_libs
    "${LibRaw_r_LIBRARIES}"
    "${QT_LIBRARIES}"
    "${EXIV2_LIBRARY}"
    "${ZLIB_LIBRARIES}"
)

if(WIN32)
    get_filename_component(LIB_PATH "${EXIV2_LIBRARY}" PATH)
    find_library(EXPAT_LIBRARY libexpat.a "${LIB_PATH}")
    find_library(ICONV_LIBRARY libiconv.a "${LIB_PATH}")
    find_library(GETTEXT_LIBRARY libintl.a "${LIB_PATH}")
    set(hdrmerge_libs
        ${hdrmerge_libs}
        "${EXPAT_LIBRARY}"
        "${GETTEXT_LIBRARY}"
        "${ICONV_LIBRARY}"
        psapi
        wsock32
        imm32
        winmm
    )
    remove_definitions(-DQT_DLL)
else()
    set(hdrmerge_libs
        "${hdrmerge_libs}"
        "${ALGLIB_LIBRARIES}"
    )
endif()

# Qt4 intermediate files
QT4_WRAP_CPP(hdrmerge_moc ${hdrmerge_qobject_headers})
QT4_ADD_TRANSLATION(hdrmerge_qm ${hdrmerge_translations})

# Generate the XML version of hdrmerge_qm
foreach(file ${hdrmerge_qm})
    get_filename_component(basename ${file} NAME)
    set(HDRMERGE_QM_XML "${HDRMERGE_QM_XML}
        <file>${basename}</file>")
endforeach()

configure_file(
    "${PROJECT_SOURCE_DIR}/data/translations.qrc.in"
    "${PROJECT_BINARY_DIR}/translations.qrc"
)

QT4_ADD_RESOURCES(hdrmerge_rsrc
    "${PROJECT_BINARY_DIR}/translations.qrc"
    data/resources.qrc
)

if(WIN32)
    add_library(alglib-objects OBJECT
        ${alglib_sources}
    )
endif()

add_library(hdrmerge-objects OBJECT
    ${hdrmerge_sources}
    ${hdrmerge_rsrc}
)

add_library(hdrmerge-gui-objects OBJECT
    ${hdrmerge_gui_sources}
    ${hdrmerge_moc}
)

if(WIN32)
    add_executable(hdrmerge ${PLATFORM}
        src/main.cpp
        src/Launcher.cpp
        $<TARGET_OBJECTS:hdrmerge-objects>
        $<TARGET_OBJECTS:hdrmerge-gui-objects>
        $<TARGET_OBJECTS:alglib-objects>
        "${PLATFORM_SOURCES}"
    )
else()
    add_executable(hdrmerge ${PLATFORM}
        src/main.cpp
        src/Launcher.cpp
        $<TARGET_OBJECTS:hdrmerge-objects>
        $<TARGET_OBJECTS:hdrmerge-gui-objects>
        "${PLATFORM_SOURCES}"
    )
endif()

target_link_libraries(hdrmerge ${STRIP} ${hdrmerge_libs})

if(WIN32)
    # Compile a target without GUI, for the .com executable
    add_executable(hdrmerge-nogui
        src/main.cpp
        src/Launcher.cpp
        $<TARGET_OBJECTS:hdrmerge-objects>
        $<TARGET_OBJECTS:alglib-objects>
        "${PLATFORM_SOURCES}"
    )
    target_link_libraries(hdrmerge-nogui ${STRIP} ${hdrmerge_libs})
    set_target_properties(hdrmerge-nogui PROPERTIES COMPILE_DEFINITIONS "NO_GUI")
    # Create the installer with makensis
    find_program(MAKENSIS_EXECUTABLE makensis.exe PATH_SUFFIXES "NSIS/Bin")
    if(MAKENSIS_EXECUTABLE)
        message(STATUS "NSIS found!")
        set(SETUP_PROG hdrmerge-setup${WIN_ARCH}-${HDRMERGE_VERSION}.exe)
        configure_file(
            "${PROJECT_SOURCE_DIR}/data/setup.nsi"
            "${PROJECT_BINARY_DIR}/setup.nsi" @ONLY
        )
        add_custom_command(OUTPUT ${SETUP_PROG}
            COMMAND "${MAKENSIS_EXECUTABLE}" -V2 setup.nsi
            MAIN_DEPENDENCY setup.nsi
            DEPENDS "${PROJECT_BINARY_DIR}/hdrmerge.exe"
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
        add_custom_target(hdrmerge-setup ALL DEPENDS "${PROJECT_BINARY_DIR}/${SETUP_PROG}")
    else()
        message(STATUS "NSIS not found!")
    endif()
elseif(APPLE)
    set_source_files_properties(images/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    install(TARGETS hdrmerge BUNDLE DESTINATION "/Applications")
else()
    if(NOT DEFINED CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR "bin")
    endif()
    message(STATUS "Install binary to: " ${CMAKE_INSTALL_BINDIR})
    install(TARGETS hdrmerge DESTINATION ${CMAKE_INSTALL_BINDIR})
    find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
    find_program(XDG-ICON-RESOURCE_EXECUTABLE xdg-icon-resource)
    if(XDG-DESKTOP-MENU_EXECUTABLE AND XDG-ICON-RESOURCE_EXECUTABLE)
        configure_file(
            "${PROJECT_SOURCE_DIR}/data/hdrmerge.desktop"
            "${PROJECT_BINARY_DIR}/hdrmerge.desktop"
        )
        install(CODE "
            execute_process(COMMAND \"${XDG-ICON-RESOURCE_EXECUTABLE}\" install --novendor --size 128 \"${PROJECT_SOURCE_DIR}/images/icon.png\" hdrmerge-icon)
            execute_process(COMMAND \"${XDG-DESKTOP-MENU_EXECUTABLE}\" install --novendor \"${PROJECT_BINARY_DIR}/hdrmerge.desktop\")"
        )
    endif()
endif()

if(Boost_FOUND)
    add_subdirectory(test)
endif()

