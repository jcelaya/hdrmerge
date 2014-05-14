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

HDRMerge fuses two or more raw images into a single raw with an extended dynamic range. It can import any raw image supported by LibRaw, and outputs a DNG 1.4 image with floating point data. The output raw is built from the less noisy pixels of the input, so that shadows maintain as much detail as possible. This tools also offers a GUI to remove 'ghosts' from the resulting image.

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

HDRMerge merges raw images directly, without development. The advantages are that it is very fast, requires little to none input from the user, and can be very optimistic on which pixels are the best ones. In fact, it can safely assume a linear response function of the camera. The resulting image then consists of the most exposed pixels from the input images that are not saturated. This ensures the lowest possible level of noise and highest detail in the output image. This is inspired in what Guillermo Luijk implemented with [Zero][1] [Noise][2].

HDRMerge also allows you to treat HDR images as any other raw image, introducing them in your normal raw development workflow. The DNG output of HDRMerge is actually a raw image with very little noise and more detail.

[1]: http://www.guillermoluijk.com/tutorial/zeronoise/index.html
[2]: http://www.guillermoluijk.com/article/virtualraw/index_en.htm

## Did you say DNG 1.4 with floating point data? What is that?

In the last revision of the DNG SDK, version 1.4, Adobe introduced the possibility of encoding the data as 16-, 24- and 32-bit floating point numbers, instead of the usual 16-bit integers. In this way, the dynamic range that can be represented with such an encoding is vastly increased. Furthermore, the floating point encoding dedicates the same number of levels to each exposure step.

The drawback is that very few programs read this format. Officially, only recent versions of Adobe products read it. I have confirmed Adobe Lightroom v5.4 myself. So, I also provide a patch for the fantastic [Rawtherapee][1] raw development program that allows it to import this format natively. Rawtherapee includes a tone mapping operator that produces great results (realistic ones, at least; if you like alien landscapes, this may be not for you).

If you want to tone-map an image resulting from HDRMerge with a program like Luminance or Photomatix, a solution would be to develop it first as a 16-bit TIFF image, with Rawtherapee or Lightroom. Then, open it with your tone-mapping program as a single image. 16-bit integers, along with a gamma correction, are usually enough to encode most HDR images with detailed shadows free of noise (after all, the dynamic range is all about [noise][2]). You can also pull the shadows up yourself during raw development.

[1]: http://rawtherapee.com/
[2]: http://theory.uchicago.edu/~ejm/pix/20d/tests/noise/index.html


# Installation:

Currently, HDRMerge is only supported in Linux. HDRMerge depends on:

* Qt (tested with version 4.8)
* LibRaw (tested with version 0.16.0)
* Exiv2 (tested with version 0.23)
* ZLib (tested with version 1.2.7)

You will need the development files of these libraries, CMake version 2.8.8 or greater and gcc 4.8 or greater.

## Building HDRMerge

The steps to compile and install HDRMerge are:

    $ mkdir build; cd build
    $ cmake ..
    $ (sudo) make install

You will probably need root access to install HDRMerge to its default location, in `/usr/local/bin`. If you want to change the path where it will be installed, you can set the CMAKE_INSTALL_PREFIX variable when you run 'cmake'. For instance, to install it in `$HOME/bin`:

    $ cmake -DCMAKE_INSTALL_PREFIX=$HOME/bin ..

# Usage:

Source images can be loaded from the Open option in the File menu, or passed as arguments in the command line. They must be made with the same camera. After loading them, HDRMerge will correct small misalignments by translation. So, if your camera allows it, you can take the shots with bracketing in burst mode. I have successfully done this just holding the camera with my hands, but using a tripod is highly recommended to obtain the best results.

Once the input images are loaded, the interface presents you with a 100% preview of the result. The selected pixels from each input image are painted with a different color. You can then pan the result to inspect it.

When some objects were moving while you took the shots, there will appear "ghosts". You can use the bottom toolbar to add or remove pixels from each image but the last one, until all the pixels that belong to a moving object only come from one of the input images. Usually, you will want to only remove pixels, starting with the first layer and then going down. Adding pixels to the first layers may result in burned areas appearing in the result image, so be careful. On the other hand, the pixels of the first layers contain less noise in the shadows. These operations can be undone and redone with the actions of the Edit menu.

Once the preview is satisfactory, the Save HDR option of the File menu generates the output DNG file. You can select the number of bits per sample (16, 24 or 32), the size of the embedded preview (full, half or no preview) and whether to save an image with the mask that was used to merge the input files. The number of bits per sample has an important impact in the output file size. As a rule of thumb, the default value of 16 bits will be enough most of the time. Empirical tests (thanks to DrSlony) show no apparent difference between 16 and 32 bits, after merging 5 exposures with 2EV steps, despite strong manipulation of shadows/mid-tones/highlights. Nevertheless, if you see some unexpected noise in the shadows of the output image, you can try 24 bits. 32 bits will almost never be necessary, but it can be selected anyway.

The program can also be run without GUI, in batch mode. This is accomplished either by providing an output file name with the "-o" switch, or by generating an automatic one with the "-a" switch. Other switches control the output parameters, refer to the output of the "--help" switch.

# Acknowledgments

I would like to thank all the people that have contributed ideas, critics and samples to improve HDRMerge. In particular, to the team of Rawtherapee.

Also, HDRMerge implements or is based on the techniques described in the following works:

1. Ward, G. (2003). Fast, robust image registration for compositing high dynamic range photographs from hand-held exposures. *Journal of graphics tools*, 8(2), 17-30.
+  Guillermo Luijk, Zero Noise, <http://www.guillermoluijk.com/tutorial/zeronoise/index.html>
+  Jens Mueller, dngconvert, <https://github.com/jmue/dngconvert>
+  Jarosz, W. (2001). Fast image convolutions. In SIGGRAPH Workshop. Code from Ivan Kuckir, <http://blog.ivank.net/fastest-gaussian-blur.html>
