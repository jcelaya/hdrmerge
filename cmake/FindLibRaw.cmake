# - Find LibRaw
# Find the LibRaw library <http://www.libraw.org>
# This module defines
#  LibRaw_VERSION_STRING, the version string of LibRaw
#  LibRaw_INCLUDE_DIR, where to find libraw.h
#  LibRaw_LIBRARIES, the libraries needed to use LibRaw (non-thread-safe)
#  LibRaw_r_LIBRARIES, the libraries needed to use LibRaw (thread-safe)
#  LibRaw_DEFINITIONS, the definitions needed to use LibRaw (non-thread-safe)
#  LibRaw_r_DEFINITIONS, the definitions needed to use LibRaw (thread-safe)
#
# Copyright (c) 2013, Pino Toscano <pino at kde dot org>
# Copyright (c) 2013, Gilles Caulier <caulier dot gilles at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    PKG_CHECK_MODULES(LibRaw libraw)
    PKG_CHECK_MODULES(LibRaw_r libraw_r)
endif()

include(FindPackageHandleStandardArgs)
mark_as_advanced(LibRaw_VERSION_STRING
                 LibRaw_INCLUDE_DIR
                 LibRaw_LIBRARIES
                 LibRaw_r_LIBRARIES
                 LibRaw_DEFINITIONS
                 LibRaw_r_DEFINITIONS
                 )
