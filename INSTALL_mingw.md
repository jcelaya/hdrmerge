# Instructions to build HDRMerge with the MinGW-w64 cross-compiler on Debian
These instructions are meant to be a guide to cross-compile HDRMerge on Debian for the Windows platform, both 32- and 64-bit editions.
They are not official in any way, only Linux is officially supported.
They work for me, so I can publish a binary version of HDRMerge for this platform with each new release.
But I will not answer any email and I will close any issue on this matter.

Throughout this guide, I will assume that you are installing your cross-compiled libraries in $HOME/usr/x86_64-w64-mingw32.
For the 32-bit version, substitute x86_64-w64-mingw32 for i686-w64-mingw32 everywhere it appears.

You will need to build/download the following libraries:

* [Qt 4.8](http://qt-project.org/)
* [LibRaw](http://www.libraw.org/)
* [zlib](http://www.zlib.net/)
* [libiconv](https://www.gnu.org/software/libiconv/)
* [libexpat](http://expat.sourceforge.net/)
* [gettext](http://www.gnu.org/software/gettext/)
* [exiv2](http://www.exiv2.org/)

## Build Qt
Unpack Qt and build it as:

    ./configure -static -xplatform win32-g++ -device-option CROSS_COMPILE=x86_64-w64-mingw32- -prefix /home/javi/usr/x86_64-w64-mingw32/Qt-4.8.6-static -opensource -qt-sql-sqlite -no-qt3support -no-xmlpatterns -no-multimedia -no-phonon -no-webkit -no-javascript-jit -no-script -no-scripttools -no-declarative -qt-zlib -qt-libtiff -qt-libpng -qt-libmng -qt-libjpeg -no-openssl -no-nis -no-cups -no-dbus -qt-freetype -make libs -nomake tools -nomake examples -nomake tests -qtlibinfix 4
    make install

## Build LibRaw
For the libraries that can be built with cmake, you can use the toolchain file that is shipped with HDRMerge.
There are two versions, one for 32-bit and another for 64-bit.
You probably do not need to change them, but they include some paths that may be different in your system.
In particular, you must set the variable PREFIX (or pass it as parameter to cmake) to where you want to cmake look for additional software.
Then, once LibRaw is uncompressed, it is built with GNU Autoconf:

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared --enable-openmp --disable-jpeg --disable-jasper --disable-lcms --disable-examples
    make install

## Build zlib
ZLib is built using a Makefile in the win32 directory:

    make -f win32/Makefile.gcc PREFIX=x86_64-w64-mingw32- BINARY_PATH=$HOME/usr/x86_64-w64-mingw32/bin INCLUDE_PATH=$HOME/usr/x86_64-w64-mingw32/include LIBRARY_PATH=$HOME/usr/x86_64-w64-mingw32/lib install

## Build libiconv
GNU libiconv is built with GNU Autoconf:

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared
    make install

## Build expat
The same for libexpat

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared
    make install

## Build gettext

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared
    make install

## Build exiv2

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared --with-zlib=$HOME/usr/x86_64-w64-mingw32
    make install

# Build HDRMerge
HDRMerge is built using cmake.
Here, be sure to set the cmake variable QT_ROOT to where you have the Qt libraries.

    mkdir build-win64
    cd build-win64
    cmake -DCMAKE_TOOLCHAIN_FILE=${HDRMERGE_PATH}/cmake/Windows64.cmake -DPREFIX=$HOME/usr/x86_64-w64-mingw32 -DQT_ROOT=$HOME/usr/x86_64-w64-mingw32/Qt-4.8.6-static ${HDRMERGE_PATH}
    make

The result of the compilation should be the binary hdrmerge.exe.    

