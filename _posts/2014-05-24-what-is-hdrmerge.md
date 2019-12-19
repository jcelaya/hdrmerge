---
layout: post
image: what-is-hdrmerge.jpg
title: What is HDRMerge?
categories: documentation
---

HDRMerge creates raw images with an extended dynamic range. It does so using either multiple exposure-bracketed raw files (any camera), or a single raw file which contains multiple exposure-bracketed frames (Fuji EXR and some Pentax cameras). It can import any raw format supported by LibRaw, and outputs a DNG 1.4 image with floating point data. The output raw is built from the less noisy pixels of the input, so that shadows maintain as much detail as possible. This tool also offers a GUI to remove 'ghosts' from the resulting image.

## Wait... Another HDR program?

Not exactly... Common HDR programs, like [Luminance HDR][1] or [Photomatix][2], actually perform two tasks:

1. Exposure merging.
2. Tone mapping.

Exposure merging involves taking the most-exposed pixels of a set of images with different exposures and combining them so as to obtain a single HDR output image with a higher dynamic range than any of the inputs. This is what HDRMerge does. Tone mapping, on the other hand, involves taking this HDR image and compressing its dynamic range so that is can be presented on a medium with a lower dynamic range, such as a computer screen or paper.

Something that many people do not realize is that these two tasks are entirely independent of each other. For instance, Luminance HDR allows you to save the (non-raw - typically TIFF, OpenEXR or Radiance RGBE) HDR image which results from exposure merging. Then, you can open this HDR image in an application which supports HDR images and apply a tone-mapping operator to squash the dynamic range while retaning detail. Likewise, HDRMerge generates an HDR image that can be later tone-mapped with another program, but this image is still raw, so the program needs to support high dynamic range raw images in the DNG format.

## So, why should I use HDRMerge?

Common HDR programs develop (demosaic) the input raw images prior to merging, performing several non-linear transformations: white balancing, highlight recovery, demosaicing and gamma correction, sharpening and denoising, among others. As a result, the merging process is complicated. It must calculate a non-linear response function of the camera and take conservative decisions on whether a pixel is useful or not. Usually, the output pixels are calculated as a weighted sum of their corresponding input pixels. Then, part of the noise of the least-exposed input image is transferred to the output, and ghost artifacts are difficult to deal with.

HDRMerge merges raw images directly, without development. The advantages are that it is very fast, requires little to no input from the user, and can be very optimistic on which pixels are the best ones. In fact, it can safely assume a linear response function of the camera. The resulting image then consists of the most-exposed pixels from the input images that are not saturated. This ensures the lowest possible level of noise and highest detail in the output image. This is inspired by what Guillermo Luijk implemented with [Zero][3] [Noise][4].

HDRMerge also allows you to treat HDR images as any other raw image, introducing them in your normal raw development workflow. The DNG output of HDRMerge is actually a raw image with very little noise and more detail.

## Did you say DNG 1.4 with floating point data? What is that?

In the last revision of the DNG SDK, version 1.4, Adobe introduced the possibility of encoding the data as 16-, 24- and 32-bit floating point numbers, instead of the usual 16-bit integers. In this way, the dynamic range that can be represented with such an encoding is vastly increased. Furthermore, the floating point encoding dedicates the same number of levels to each exposure step.

The drawback is that very few programs read this format. [RawTherapee][5] (as for v4.1) and [darktable][6] support these formats, as do recent versions of Adobe products.

If you want to tone-map an image resulting from HDRMerge with a program like Luminance HDR or Photomatix, a solution would be to develop it first (using RawTherapee, darktable or Lightroom) as a 16- or 32-bit TIFF image. Then, open the TIFF with your tone-mapping program. 16-bit integers, along with a gamma correction, are usually enough to encode most HDR images with detailed shadows free of noise (after all, the dynamic range is all about [noise][7]). You can also pull the shadows up yourself during raw development.

[1]: http://qtpfsgui.sourceforge.net/
[2]: https://www.hdrsoft.com/
[3]: http://www.guillermoluijk.com/tutorial/zeronoise/
[4]: http://www.guillermoluijk.com/article/virtualraw/index_en.htm
[5]: https://www.rawtherapee.com/
[6]: https://www.darktable.org/
[7]: http://theory.uchicago.edu/~ejm/pix/20d/tests/noise/index.html
