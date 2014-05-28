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
* [exiv2](http://www.exiv2.org/)

## Download Qt binary builds
You probably do not want to build Qt 4.8 from scratch.
So, you can download a pre-compiled version from the MinGW-builds project:
<http://sourceforge.net/projects/mingwbuilds/files/external-binary-packages/Qt-Builds/>

## Build LibRaw
For the libraries that can be built with cmake, you can use the toolchain file that is shipped with HDRMerge.
There are two versions, one for 32-bit and another for 64-bit.
You probably do not need to change them, but they include some paths that may be different in your system.
In particular, you must set the variable PREFIX (or pass it as parameter to cmake) to where you want to cmake look for additional software.
Then, once LibRaw is uncompressed, it is built as:

    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=${HDRMERGE_PATH}/cmake/Windows64.cmake -DPREFIX=$HOME/usr/x86_64-w64-mingw32 -DENABLE_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=$HOME/usr/x86_64-w64-mingw32 ..
    make install

It fails to install FindLibRaw.cmake, but it is included with hdrmerge.

## Build zlib
ZLib is built using a Makefile in the win32 directory:

    make -f win32/Makefile.gcc PREFIX=x86_64-w64-mingw32- BINARY_PATH=$HOME/usr/x86_64-w64-mingw32/bin INCLUDE_PATH=$HOME/usr/x86_64-w64-mingw32/include LIBRARY_PATH=$HOME/usr/x86_64-w64-mingw32/lib SHARED_MODE=1 install

## Build libiconv
GNU libiconv is built with GNU Autoconf:

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32
    make install

## Build expat
The same for libexpat

    ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32
    make install

## Build exiv2
Finally, exiv2 can be built with cmake again:

    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=${HDRMERGE_PATH}/cmake/Windows64.cmake -DPREFIX=$HOME/usr/x86_64-w64-mingw32 -DCMAKE_INSTALL_PREFIX=$HOME/usr/x86_64-w64-mingw32 ..
    make install

# Build HDRMerge
HDRMerge is built using cmake.
Here, be sure to set the cmake variable QT_ROOT to where you have uncompressed the Qt libraries.

    mkdir build-win64
    cd build-win64
    cmake -DCMAKE_TOOLCHAIN_FILE=${HDRMERGE_PATH}/cmake/Windows64.cmake -DPREFIX=$HOME/usr/x86_64-w64-mingw32 -DQT_ROOT=$HOME/usr/x86_64-w64-mingw32/Qt64-4.8.4 ..
    make

The result of the compilation should be the binary hdrmerge.exe.    

## Bundle the result in a single zip
All the previous steps build each library as a DLL.
They must be in the same directory as hdrmerge.exe to execute it.
The list of libraries with their paths is:

* /usr/lib/gcc/x86_64-w64-mingw32/4.8/libgcc_s_sjlj-1.dll
* /usr/lib/gcc/x86_64-w64-mingw32/4.8/libgomp-1.dll
* /usr/lib/gcc/x86_64-w64-mingw32/4.8/libstdc++-6.dll
* /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll
* $HOME/usr/x86_64-w64-mingw32/bin/libexpat-1.dll
* $HOME/usr/x86_64-w64-mingw32/bin/libiconv-2.dll
* $HOME/usr/x86_64-w64-mingw32/bin/zlib1.dll
* $HOME/usr/x86_64-w64-mingw32/bin/libexiv2.dll
* $HOME/usr/x86_64-w64-mingw32/bin/libraw.dll
* $HOME/usr/x86_64-w64-mingw32/Qt64-4.8.4/bin/QtCore4.dll
* $HOME/usr/x86_64-w64-mingw32/Qt64-4.8.4/bin/QtGui4.dll
* $HOME/usr/x86_64-w64-mingw32/Qt64-4.8.4/plugins/imageformats/qjpeg4.dll

Note that `qjpeg4.dll` must not be in the same directory as `hdrmerge.exe`, but in a subdirectory named `imageformats`. Otherwise, Qt will not detect it as an image format plugin.

