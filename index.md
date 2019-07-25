---
layout: default
image: index.jpg
title: HDRMerge
---
HDRMerge combines two or more raw images into a single raw with an extended dynamic range. It can import any raw image supported by LibRaw, and outputs a DNG 1.4 image with floating point data. The output raw is built from the less noisy pixels of the input, so that shadows maintain as much detail as possible. This tool also offers a GUI to remove 'ghosts' from the resulting image. [Discover more about HDRMerge]({{ site.baseurl }}{% post_url 2014-05-24-what-is-hdrmerge %})

HDRMerge GitHub project

# Downloads

HDRMerge is supported in Linux, Windows and macOS.

## Latest Stable Release

This is [the latest version]({{ site.github }}/releases/latest), x ({{ site.github.releases.name }}) y ({{ site.github.releases.latest.name }}) z ({{ site.github.latest_releases.name }}), recommended for everyday use.

* [Windows installer]({{ site.github.latest_release }})
* Linux AppImage (not yet)
* [macOS DMG]({{ site.github.latest_release }})
* [Source code]({{ site.github.latest_release }})

## Latest Development Builds

These are builds of the latest development code, recommended for testing and bug reporting but not for everyday use.

* Windows installer (not yet)
* [Linux AppImage]({{ site.github }}/releases/tag/nightly)
* macOS DMG (not yet)
* [Source code]({{ site.github }}/releases/tag/nightly)

# Bugs and Feature Requests

File bugs and feature requests in [GitHub]({{ site.github.issues_url }}). Make sure to provide the necessary information: which version of HDRMerge you're using, the full contents of any error messages, and a link to your sample raw files.


# Getting Started

You may want to read the [manual]({{ site.baseurl }}{% post_url 2014-07-11-user-manual %}), or jump directly to the command line help with `hdrmerge --help`.


# Changelog

These are the most significant improvements by version. For a detailed changelog, see the [git commit history]({{ site.github }}/commits/master).

## v0.5

* Support for macOS.
* Multiple bug-fixes.

## v0.4.5

* Better compatibility with other programs, by producing a DNG file that maintains the original layout: frame and active area sizes, black and white levels, etc. *Note that, if you use RawTherapee, you need v4.1.23 or higher to open these files.*
* Batch mode in command line! Merge several sets of HDR images at once.
* Creation of menu launchers and a Windows installer.
* Support for CYGM and Fujifilm X-Trans sensors (experimental).
* Several bug-fixes.
* Improved accuracy and performance.

## v0.4.4

* Better support for more camera models.
* Better rendering of the embedded preview image.
* Change the edit brush radius with Alt+Mouse wheel.
* Several bugfixes.
  * The original embedded preview is not included in the output anymore.
  * Fixed some glitches with the edit tools.

## v0.4.3

* Fix segmentation fault error painting the preview of some rotated images.
* Fix DateTime tag in Windows hosts.

## v0.4.2

* Improved GUI:
  * A slider to control the brush radius.
  * A slider to control the preview exposure.
  * Movable toolbars.
  * Layer selector with color codes.
  * Improved brush visibility on different backgrounds.
  * Posibility of saving the output options.
* First release with a Windows version, both 32- and 64-bit.

## v0.4.1

* Bugfixes release

## v0.4

* Great performance improvements with OpenMP.
* Not depend anymore on DNG & XMP SDK! Windows and Mac version soon...
* More robust MBT alignment.
* More control on the logging output.
* The user may disable alignment and/or cropping. This is most useful to obtain an image of the same size as the inputs. Some programs have this requirement to apply a flat-field image, for instance.

## v0.3

The first public version of HDRMerge.

* Supports most raw format supported by LibRaw (No foveon of Fuji formats for the moment).
* Automatic alignment of small translations.
* Automatic crop to the optimal size.
* Automatic merge mask creation. The mask identifies the best source image for each pixel of the output.
* Editable merge mask, to manually select pixels from specific source images.
* Writes DNG files with 16, 24 and 32 bits per pixel.
* Writes full, half or no preview to the output image.
* Copies the EXIF data from the least exposed source image.


# Donations

Do you like HDRMerge? Do you want to keep it under development? You can make a donation through [Flattr](https://flattr.com/) or [PayPal](https://www.paypal.com/):

<form action="https://www.paypal.com/cgi-bin/webscr" method="post" target="_top" style="margin: 0; padding: 0;">
	<a href="https://flattr.com/submit/auto?user_id=jcelaya&url=http%3A%2F%2Fjcelaya.github.io%2Fhdrmerge%2F&title=HDRMerge" target="_blank" style="border: 0px">
    	<img src="//api.flattr.com/button/flattr-badge-large.png" alt="Flattr this" title="Flattr this" style="padding-bottom: 12px;" border="0"></a> 
    <input type="hidden" name="cmd" value="_s-xclick">
    <input type="hidden" name="hosted_button_id" value="AB3CAVRH4S24C">
    <input type="image" src="https://www.paypalobjects.com/en_GB/i/btn/btn_donate_LG.gif" border="0" name="submit"
        alt="PayPal – The safer, easier way to pay online." style="display: inline;">
    <img alt="" border="0" src="https://www.paypalobjects.com/es_ES/i/scr/pixel.gif" width="1" height="1">
</form>


# License

HDRMerge is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

HDRMerge is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.


# Acknowledgments

HDRMerge is what it is thanks to all the people that have contributed ideas, critics and samples to improve it. In particular, thanks to the team of [RawTherapee](http://rawtherapee.com/). Also, HDRMerge implements or is based on the techniques described in the following works:

1. Ward, G. (2003). Fast, robust image registration for compositing high dynamic range photographs from hand-held exposures. *Journal of graphics tools*, 8(2), 17-30.
+  Guillermo Luijk, Zero Noise, <http://www.guillermoluijk.com/tutorial/zeronoise/index.html>
+  Jens Mueller, dngconvert, <https://github.com/jmue/dngconvert>
+  Jarosz, W. (2001). Fast image convolutions. In SIGGRAPH Workshop. Code from Ivan Kuckir, <http://blog.ivank.net/fastest-gaussian-blur.html>
