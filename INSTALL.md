# Compilation
This document explains how to compile HDRMerge.

## Dependencies
- [ALGLIB](http://www.alglib.net/)
- [Boost](http://www.boost.org/)
- [CMake](https://cmake.org/) 3.4
- [exiv2](http://www.exiv2.org/)
- [gettext](http://www.gnu.org/software/gettext/)
- [libexpat](http://expat.sourceforge.net/)
- [libiconv](https://www.gnu.org/software/libiconv/)
- [LibRaw](http://www.libraw.org/)
- [Qt](https://www.qt.io/) 5.6
- [zlib](http://www.zlib.net/)

Install the dependencies and proceed to the next section.

### Arch and derivatives
```bash
sudo pacman -Syy
sudo pacman -S --needed cmake libraw pacaur qt5
pacaur -S alglib
```

### Debian/Ubuntu and derivatives
```bash
sudo apt update
sudo apt install build-essential cmake git libalglib-dev libboost-all-dev libexiv2-dev libexpat-dev libraw-dev qt5-default zlib1g-dev
```

### Gentoo and derivatives
```bash
sudo emerge -uva sci-libs/alglib dev-libs/boost dev-util/cmake media-gfx/exiv2 dev-vcs/git media-libs/libraw sys-devel/gettext dev-libs/expat virtual/libiconv dev-qt/qtcore:5 sys-libs/zlib
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
./scripts/build-hdrmerge
```

HDRMerge will be ready for use in `~/programs/hdrmerge/hdrmerge`
You have finished.

## Compilation in Linux for Windows
These instructions are meant to be a guide to cross-compile HDRMerge on Debian for the Windows platform, both 32- and 64-bit editions.

It is assumed that you are installing your cross-compiled libraries in `$HOME/usr/x86_64-w64-mingw32`.
For the 32-bit version, substitute `x86_64-w64-mingw32` for `i686-w64-mingw32` everywhere it appears.

You will need to build/download the following libraries:
- Qt
_**TODO**: Update to Qt5_
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
_**TODO**: Is QT_ROOT still valid in Qt5?_
Set the CMake variable `QT_ROOT` to where you have the Qt libraries installed.

```bash
mkdir build-win64
cd build-win64
cmake -DCMAKE_TOOLCHAIN_FILE=${HDRMERGE_PATH}/cmake/Windows64.cmake -DPREFIX=$HOME/usr/x86_64-w64-mingw32 -DQT_ROOT=$HOME/usr/x86_64-w64-mingw32/Qt-4.8.6-static ${HDRMERGE_PATH}
make
```

The result of the compilation should be the binaries `hdrmerge.exe` and `hdrmerge-nogui.exe`.
You have finished.

## Compilation in macOS
The first step is to get all the dependencies as well as the source code:

**NOTE:** xCode is not required but recommended. The Command Line tools that are implicitly installed with Homebrew are sufficient.

1. a. Install the dependencies using [Homebrew](https://brew.sh):
``` bash
brew install cmake boost exiv2 libraw qt libomp
```
1. b. alglib is not available on brew (for now) so [Download ALGLIB 3.15.0 for C++](http://www.alglib.net/download.php) and extract to ~/alglib manually or:
``` bash
mkdir ~/alglib && cd ~/alglib
curl http://www.alglib.net/translator/re/alglib-3.15.0.cpp.gpl.zip --output ~/alglib/ALGLIB.zip
unzip ~/alglib/ALGLIB.zip -d ~/alglib && rm ~/alglib/ALGLIB.zip
```

1. c. Clone HDRMerge into ~/hdrmerge and checkout master branch
``` bash
git clone https://github.com/jcelaya/hdrmerge.git ~/hdrmerge
cd ~/hdrmerge
git checkout master
```
2. Make and go to the build directory.
``` bash
mkdir ~/hdrmerge/build && cd ~/hdrmerge/build
```
3. Issue cmake command
``` bash
cmake .. -DQt5_DIR=/usr/local/Cellar/Qt/5.12.3/lib/cmake/Qt5 -DCMAKE_BUILD_TYPE=Release -DOpenMP_C_FLAGS=-fopenmp=libiomp5 -DOpenMP_CXX_FLAGS=-fopenmp=libiomp5 -DOpenMP_C_LIB_NAMES="libiomp5" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_libiomp5_LIBRARY="/usr/local/lib/libomp.dylib" -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /usr/local/lib/libomp.dylib -I/usr/local/include" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /usr/local/lib/libomp.dylib -I/usr/local/include" -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk" -DALGLIB_ROOT=$HOME/alglib/cpp -DALGLIB_INCLUDES=$HOME/alglib/cpp/src -DALGLIB_LIBRARIES=$HOME/alglib/cpp/src -DCMAKE_INSTALL_BINDIR=$HOME/hdrmerge/build/install
```
If the command fails then make sure the version of Qt from the command matches the one you have installed.

4. Compile
``` bash
sudo make -j4 install
```

5. Copy two of the dependencies into Frameworks.
``` bash
sudo mkdir ~/hdrmerge/build/install/hdrmerge.app/Contents/Frameworks
sudo cp /usr/local/lib/libomp.dylib ~/hdrmerge/build/install/hdrmerge.app/Contents/Frameworks/.
sudo cp /usr/local/lib/libexiv2.dylib ~/hdrmerge/build/install/hdrmerge.app/Contents/Frameworks/.
```

6. Run Qt5's macdeployqt (adapt to your Qt version)
``` bash
sudo /usr/local/Cellar/qt/5.12.3/bin/macdeployqt ~/hdrmerge/build/install/hdrmerge.app -no-strip -verbose=1
```

7. Install an rpath
``` bash
sudo install_name_tool -add_rpath "@executable_path/../Frameworks" ~/hdrmerge/build/install/hdrmerge.app/Contents/MacOS/hdrmerge
```

8. Make the .dmg for further distribution (optional)
``` bash
sudo hdiutil create -ov -srcfolder ~/hdrmerge/build/install/hdrmerge.app ~/hdrmerge/build/install/HDRMerge.dmg
```

That was it! You can move the hdrmerge.app to you applications folder and start using it!
``` bash
cp -r ~/hdrmerge/build/install/hdrmerge.app /Applications/HDRMerge.app
```
