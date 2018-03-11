# Compilation
This document explains how to compile HDRMerge.

## Dependencies
- [ALGLIB](http://www.alglib.net/)
- [Boost](http://www.boost.org/)
- [CMake](https://cmake.org/)
- [exiv2](http://www.exiv2.org/)
- [gettext](http://www.gnu.org/software/gettext/)
- [libexpat](http://expat.sourceforge.net/)
- [libiconv](https://www.gnu.org/software/libiconv/)
- [LibRaw](http://www.libraw.org/)
- [Qt](http://qt-project.org/) 4.8
- [zlib](http://www.zlib.net/)

Install the dependencies and proceed to the next section.

### Arch and derivatives
```bash
sudo pacman -Syy
sudo pacman -S --needed cmake libraw pacaur qt4
pacaur -S alglib
```

### Debian/Ubuntu and derivatives
```bash
sudo apt update
sudo apt install build-essential cmake git libalglib-dev libboost-all-dev libexiv2-dev libexpat-dev libraw-dev qt4-default zlib1g-dev
```

### Gentoo and derivatives
```bash
sudo emerge -uva sci-libs/alglib dev-libs/boost dev-util/cmake media-gfx/exiv2 dev-vcs/git media-libs/libraw sys-devel/gettext dev-libs/expat virtual/libiconv dev-qt/qtcore:4 sys-libs/zlib
```

## Compilation in Windows
- Download ALGLIB from http://www.alglib.net/download.php and set the base path in `ALGLIB_ROOT`.
- Install NSIS from http://nsis.sourceforge.net/Download
  If you install it to a custom folder, make sure it is accessible from your `PATH` environment variable.

Install all other dependencies.

After the `cmake` process, open `${PROJECT_BINARY_DIR}\setup.nsi` and replace all backslashes with forward slashes in the following:
- All `MUI_` variables.
- The lines following `File "hdrmerge.exe"`

Now you can `make install`.

You have finished.

## Compilation in Linux for Linux
Once you have installed the dependencies, run the following to clone and compile HDRMerge:
```bash
mkdir ~/programs
git clone https://github.com/jcelaya/hdrmerge.git ~/programs/code-hdrmerge
cd ~/programs/code-hdrmerge
./build-hdrmerge
```

HDRMerge will be ready for use in `~/programs/hdrmerge/hdrmerge`
You have finished.

## Compilation in Linux for Windows
These instructions are meant to be a guide to cross-compile HDRMerge on Debian for the Windows platform, both 32- and 64-bit editions.

It is assumed that you are installing your cross-compiled libraries in `$HOME/usr/x86_64-w64-mingw32`.
For the 32-bit version, substitute `x86_64-w64-mingw32` for `i686-w64-mingw32` everywhere it appears.

You will need to build/download the following libraries:
- Qt
Unpack Qt and build it as:
  ```bash
  ./configure -static -xplatform win32-g++ -device-option CROSS_COMPILE=x86_64-w64-mingw32- -prefix $HOME/usr/x86_64-w64-mingw32/Qt-4.8.6-static -opensource -qt-sql-sqlite -no-qt3support -no-xmlpatterns -no-multimedia -no-phonon -no-webkit -no-javascript-jit -no-script -no-scripttools -no-declarative -qt-zlib -qt-libtiff -qt-libpng -qt-libmng -qt-libjpeg -no-openssl -no-nis -no-cups -no-dbus -qt-freetype -make libs -nomake tools -nomake examples -nomake tests -qtlibinfix 4
  make install
  ```
- LibRaw
For the libraries that can be built with CMake, you can use the toolchain file that is shipped with HDRMerge. There are two versions, one for 32-bit and another for 64-bit. You probably do not need to change them, but they include some paths that may be different in your system. In particular, you must set the variable `PREFIX` (or pass it as parameter to CMake) to where you want CMake to look for additional software.
Once LibRaw is uncompressed, it is built with GNU Autoconf:
  ```bash
  ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared --enable-openmp --disable-jpeg --disable-jasper --disable-lcms --disable-examples
  make install
  ```
- zlib
ZLib is built using a Makefile in the win32 directory:
  ```bash
  make -f win32/Makefile.gcc PREFIX=x86_64-w64-mingw32- BINARY_PATH=$HOME/usr/x86_64-w64-mingw32/bin INCLUDE_PATH=$HOME/usr/x86_64-w64-mingw32/include LIBRARY_PATH=$HOME/usr/x86_64-w64-mingw32/lib install
  ```
- libiconv
GNU libiconv is built with GNU Autoconf:
  ```bash
  ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared
  make install
  ```
- libexpat
libexpat is built with GNU Autoconf:
  ```bash
  ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared
  make install
  ```
- gettext
  ```bash
  ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared
  make install
  ```
- Exiv2
  ```bash
  ./configure --prefix=$HOME/usr/x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --disable-shared --with-zlib=$HOME/usr/x86_64-w64-mingw32
  make install
  ```

Once you have the dependencies installed, you are ready to compile HDRMerge.
Set the CMake variable `QT_ROOT` to where you have the Qt libraries installed.

```bash
mkdir build-win64
cd build-win64
cmake -DCMAKE_TOOLCHAIN_FILE=${HDRMERGE_PATH}/cmake/Windows64.cmake -DPREFIX=$HOME/usr/x86_64-w64-mingw32 -DQT_ROOT=$HOME/usr/x86_64-w64-mingw32/Qt-4.8.6-static ${HDRMERGE_PATH}
make
```

The result of the compilation should be the binaries `hdrmerge.exe` and `hdrmerge-nogui.exe`.
You have finished.
