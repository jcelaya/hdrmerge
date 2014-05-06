HDRMerge - Raw exposures merging software.
Copyright 2012 Javier Celaya
jcelaya@gmail.com

This file is part of HDRMerge.

HDRMerge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HDRMerge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.


# What is HDRMerge?

HDRMerge fuses two or more raw images into a single raw with an extended dynamic range. It can import any raw image supported by LibRaw, and outputs a DNG 1.4 image with 32-bit float data. The output raw is built from the less noisy pixels of the input, so that shadows maintain as much detail as possible. This tools also offers a GUI to remove 'ghosts' from the resulting image.

## Wait... Another HDR program??

Not exactly... Common HDR programs, like [Luminance HDR][1] or [Photomatix][2], actually perform two tasks:

* Exposure merging.
* Tone mapping.

Exposure merging consists in taking the best pixels of a set of images with different exposures and obtain an output image with a higher dynamic range than any of the inputs. This is what HDRMerge does. Tone mapping consists in squeezing an HDR image to present it with all its detail in low dynamic range devices, like screens or paper. Usually, as a result, shadows are pulled up and local contrast is enhanced.

Something that many people do not realize is that these two tasks are totally independent from each other. For instance, Luminance allows you to save the HDR image that results from the merging task. Then, you can load it (or any other HDR image) later to apply any tone mapping operator that Luminance implements. Likewise, HDRMerge generates an HDR image that can be later tone-mapped with another program.

[1]: http://qtpfsgui.sourceforge.net/
[2]: http://www.hdrsoft.com/

## So, why should I use HDRMerge?

Common HDR programs develop the input raw images prior to the merging task, performing several non-linear transformations on them: white balance, highlights recovery, *demosaicing* and gamma correction, among others. As a result, the merging process is complicated. It must calculate a non-linear response function of the camera and take conservative decisions on whether a pixel is useful or not. Usually, the output pixels are calculated as a weighted sum of their corresponding input pixels. Then, part of the noise of the least exposed input images is transferred to the output, and ghost artifacts are difficult to deal with.

HDRMerge merges raw images directly, without development. The advantages are that it is very fast, requires little to none input from the user, and can be very optimistic on which pixels are the best ones. In fact, it can safely assume a linear response function of the camera. The resulting image then consists of the most exposed pixels from the input images that are not saturated. This ensures the lowest possible level of noise in the output image. This is inspired in what Guillermo Luijk implemented with [Zero][1] [Noise][2].

HDRMerge also allows you to treat HDR images as any other raw image, introducing them in your normal raw development workflow. The DNG output of HDRMerge is actually a raw image with very little noise and more detail.

[1]: http://www.guillermoluijk.com/tutorial/zeronoise/index.html
[2]: http://www.guillermoluijk.com/article/virtualraw/index_en.htm

## Did you say DNG 1.4 with 32-bit float data? What is that?

In the last revision of the DNG SDK, version 1.4, Adobe introduced the possibility of encoding the data as 32-bit floating point numbers, instead of the usual 16-bit integers. In this way, the dynamic range that can be represented with such an encoding is vastly increased. Furthermore, it dedicates the same number of levels to each exposure step.

The drawback is that very few programs read this format. Officially, only recent versions of Adobe products read it. I have confirmed Adobe Lightroom v5.4 myself. So, I also provide a patch for the fantastic [Rawtherapee][1] raw development program that allows it to import this format natively. Rawtherapee includes a tone mapping operator that produces great results (realistic ones, at least; if you like alien landscapes, this may be not for you).

If you want to tone-map an image resulting from HDRMerge with a program like Luminance or Photomatix, a solution would be to develop it first as a 16-bit TIFF image, with Rawtherapee or Lightroom. Then, open it with your tone-mapping program as a single image. 16-bit integers, along with a gamma correction, are usually enough to encode most HDR images with detailed shadows free of noise (after all, the dynamic range is all about [noise][2]). You can also pull the shadows up yourself during raw development.

[1]: http://rawtherapee.com/
[2]: http://theory.uchicago.edu/~ejm/pix/20d/tests/noise/index.html


# Installation:

Currently, HDRMerge is only supported in Linux. HDRMerge depends on:

* Qt (tested with version 4.8)
* LibRaw (tested with version 0.16.0)
* Exiv2 (tested with version 0.23)
* DNG SDK 1.4 (its source code is included with HDRMerge)
* XMP SDK (tested with version CC 2013.06)

You will need the development files of these libraries, CMake version 2.8.8 or greater and gcc 4.8 or greater.

## Building the XMP SDK

You can obtain the Adobe XMP SDK from <http://www.adobe.com/devnet/xmp.html>. To build it:

1. Unzip the sdk in the third_party directory.
+  Rename the XMP SDK directory as xmp_sdk.
+  Cd into xmp_sdk/third_party and follow the instructions to download libexpat and zlib.
+  Cd into xmp_sdk/build and run 'make StaticAll'. Known problems:
    - Depending on your version of gcc, it may give lots of warnings, just ignore them.
    - It may fail at *shared/SharedConfig_Common.cmake*, comment out the lines failing.
    - Also, *XMPFiles/source/NativeMetadataSupport/ValueObject.h* should include *string.h*.
+  Move or symlink _public/libraries/*/release/staticXMP{Core,Files}.ar_ to
    _public/libraries/libXMP{Core,Files}.a_

## Building HDRMerge

Once the XMP SDK is compiled, the steps to compile and install HDRMerge are:

    $ mkdir build; cd build
    $ cmake -DCMAKE_INSTALL_PREFIX=you_choose
    $ (sudo) make install

It will only install the binary program. You will probably need root access to install HDRMerge to its default location, in */usr/local/bin*.



# Usage:

Source exposures can be loaded from the Open option in the File menu, or passed as arguments in the command line. They must be made with the same camera. After loading them, HDRMerge will correct small misalignments by translation. So, if your camera allows it, you can take the shots with bracketing in burst mode. I have successfully done this just holding the camera with my hands, but using a tripod is highly recommended.

Once the input images are loaded, the interface presents you with a 100% preview of the result. The pixels selected from each input image are painted with a different color. You can then pan the result to inspect it.

When some objects were moving while you took the shots, there will appear "ghosts". You can use the bottom toolbar to add or remove pixels from each image but the last one, until all the pixels that belong to a moving object only come from one of the input images. These operations can be undone and redone with the actions of the Edit menu.

Once the preview is satisfactory, the Save HDR option of the File menu generates the output DNG file.

The program can be run without GUI when no ghosts appear in the image. This is accomplished either by providing an output file name with the "-o" switch, or by generating an automatic one with the "-a" switch.
