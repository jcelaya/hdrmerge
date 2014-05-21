# This is an example toolchain file to cross-compile for Windows 64-bit targets.
# It is based on the Debian's mingw-w64 package.

# this one is important
SET(CMAKE_SYSTEM_NAME Windows)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /usr/bin/x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER  /usr/bin/x86_64-w64-mingw32-windres)

# here is the target environment located
set(HOME /home/javi)
SET(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32 ${HOME}/usr/x86_64-w64-mingw32)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(QT_ROOT ${HOME}/usr/x86_64-w64-mingw32/Qt64-4.8.4)

set(QT_QTCORE_LIBRARY ${QT_ROOT}/lib/libQtCore4.a)
set(QT_QTCORE_LIBRARY_RELEASE ${QT_QTCORE_LIBRARY})
set(QT_QTCORE_INCLUDE_DIR ${QT_ROOT}/include/QtCore)

set(QT_QTGUI_LIBRARY ${QT_ROOT}/lib/libQtGui4.a)
set(QT_QTGUI_LIBRARY_RELEASE ${QT_QTGUI_LIBRARY})
set(QT_QTGUI_INCLUDE_DIR ${QT_ROOT}/include/QtGui)

set(QT_LIBRARIES ${QT_QTCORE_LIBRARY_RELEASE} ${QT_QTGUI_LIBRARY_RELEASE})

