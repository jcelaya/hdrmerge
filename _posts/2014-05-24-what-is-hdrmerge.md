---
layout: post
image: what-is-hdrmerge.jpg
title: What is HDRMerge?
categories: documentation
---
[HDRMerge](https://github.com/jcelaya/hdrmerge) combines two or more raw images into a single raw with an extended dynamic range. It can import any raw image supported by LibRaw, and outputs a DNG 1.4 image with floating point data. The output raw is built from the less noisy pixels of the input, so that shadows maintain as much detail as possible. This tool also offers a GUI to remove 'ghosts' from the resulting image.

## Wait... Another HDR program??

Not exactly... Common HDR programs, like [Luminance HDR][1] or [Photomatix][2], actually perform two tasks:

* Exposure merging.
* Tone mapping.

Exposure merging consists in taking the best pixels of a set of images with different exposures and obtain an output image with a higher dynamic range than any of the inputs. This is what HDRMerge does. Tone mapping consists in squeezing an HDR image to present it with all its detail in low dynamic range devices, like screens or paper. Usually, as a result, shadows are pulled up and local contrast is enhanced.

Something that many people do not realize is that these two tasks are totally independent from each other. For instance, Luminance allows you to save the HDR image that results from the merging task. Then, you can load it (or any other HDR image) later to apply any tone mapping operator that Luminance implements. Likewise, HDRMerge generates an HDR image that can be later tone-mapped with another program.

## So, why should I use HDRMerge?

Common HDR programs develop the input raw images prior to the merging task, performing several non-linear transformations on them: white balance, highlights recovery, *demosaicing* and gamma correction, among others. As a result, the merging process is complicated. It must calculate a non-linear response function of the camera and take conservative decisions on whether a pixel is useful or not. Usually, the output pixels are calculated as a weighted sum of their corresponding input pixels. Then, part of the noise of the least exposed input images is transferred to the output, and ghost artifacts are difficult to deal with.

HDRMerge merges raw images directly, without development. The advantages are that it is very fast, requires little to none input from the user, and can be very optimistic on which pixels are the best ones. In fact, it can safely assume a linear response function of the camera. The resulting image then consists of the most exposed pixels from the input images that are not saturated. This ensures the lowest possible level of noise and highest detail in the output image. This is inspired in what Guillermo Luijk implemented with [Zero][3] [Noise][4].

HDRMerge also allows you to treat HDR images as any other raw image, introducing them in your normal raw development workflow. The DNG output of HDRMerge is actually a raw image with very little noise and more detail.

## Did you say DNG 1.4 with floating point data? What is that?

In the last revision of the DNG SDK, version 1.4, Adobe introduced the possibility of encoding the data as 16-, 24- and 32-bit floating point numbers, instead of the usual 16-bit integers. In this way, the dynamic range that can be represented with such an encoding is vastly increased. Furthermore, the floating point encoding dedicates the same number of levels to each exposure step.

The drawback is that very few programs read this format. Officially, only recent versions of Adobe products read it. I have confirmed Adobe Lightroom v5.4 myself. So, I also provided a patch for the fantastic [RawTherapee][5] raw development program that allows it to import this format natively. This functionality is available from [its repository code][6] and will be publicly released in version 4.1. RawTherapee includes a tone mapping operator that produces great results (realistic ones, at least; if you like alien landscapes, this may be not for you).

If you want to tone-map an image resulting from HDRMerge with a program like Luminance or Photomatix, a solution would be to develop it first as a 16-bit TIFF image, with RawTherapee or Lightroom. Then, open it with your tone-mapping program as a single image. 16-bit integers, along with a gamma correction, are usually enough to encode most HDR images with detailed shadows free of noise (after all, the dynamic range is all about [noise][7]). You can also pull the shadows up yourself during raw development.

[1]: http://qtpfsgui.sourceforge.net/
[2]: http://www.hdrsoft.com/
[3]: http://www.guillermoluijk.com/tutorial/zeronoise/index.html
[4]: http://www.guillermoluijk.com/article/virtualraw/index_en.htm
[5]: http://rawtherapee.com/
[6]: http://code.google.com/p/rawtherapee/
[7]: http://theory.uchicago.edu/~ejm/pix/20d/tests/noise/index.html
