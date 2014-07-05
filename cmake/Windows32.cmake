# This is an example toolchain file to cross-compile for Windows 32-bit targets.
# It is based on the Debian's mingw-w64 package.

# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
SET(WIN_ARCH "")

# specify the cross compiler
SET(CMAKE_C_COMPILER   /usr/bin/i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER  /usr/bin/i686-w64-mingw32-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32 ${PREFIX})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Some flags so that FindQt4.cmake finds the correct libs
if(QT_ROOT)
	set(QT_QTCORE_LIBRARY_RELEASE ${QT_ROOT}/lib/libQtCore4.a)
	set(QT_QTCORE_INCLUDE_DIR ${QT_ROOT}/include/QtCore)
	set(QT_HEADERS_DIR ${QT_ROOT}/include)
endif(QT_ROOT)

