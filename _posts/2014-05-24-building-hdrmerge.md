---
layout: post
image: museum.jpg
title: Building HDRMerge
categories: internals
---
# Installation:

Currently, HDRMerge is only _officially_ supported in Linux, but binaries for Windows are also provided. A short guide on how to cross-compile it for Windows is included in the *INSTALL_mingw.md* file. HDRMerge depends on:

* Qt (tested with version 4.8)
* LibRaw (tested with version 0.16.0)
* Exiv2 (tested with version 0.23)
* ZLib (tested with version 1.2.7)

You will need the development files of these libraries, CMake version 2.8.8 or greater and gcc 4.8 or greater. Optionally, HDRMerge can use OpenMP to increase its performance, and Boost::Test to compile the unit tests.

## Building HDRMerge
First, download HDRMerge from <{{ site.github }}/releases/latest>. The steps to compile and install HDRMerge are:

    $ mkdir build; cd build
    $ cmake ..
    $ (sudo) make install

You will probably need root access to install HDRMerge to its default location, in `/usr/local/bin`. If you want to change the path where it will be installed, you can set the CMAKE_INSTALL_PREFIX variable when you run 'cmake'. For instance, to install it in `$HOME/bin`:

    $ cmake -DCMAKE_INSTALL_PREFIX=$HOME/bin ..
