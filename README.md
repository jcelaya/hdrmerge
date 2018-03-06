# HDRMerge

HDRMerge combines two or more raw images into a single raw with an extended dynamic range. It can import any raw image supported by LibRaw, and outputs a DNG 1.4 image with floating point data. The output raw is built from the less noisy pixels of the input, so that shadows maintain as much detail as possible. This tool also offers a GUI to remove ghosts from the resulting image.

## Download & Installation
Find the latest builds on our [releases page](https://github.com/jcelaya/hdrmerge/releases).

Linux users can get HDRMerge from their package manager. If your package manager does not ship the latest version of HDRMerge, file a bug report using your distribution's bug tracker asking them to ship the latest version.

## Compilation
If you would like to compile HDRMerge yourself, follow the instructions in the `INSTALL.md` file.

## Usage
Source images can be loaded from the Open option in the File menu, or passed as arguments in the command line. They must be made with the same camera. After loading them, HDRMerge will correct small misalignments by translation. So, if your camera allows it, you can take the shots with bracketing in burst mode. I have successfully done this just holding the camera with my hands, but using a tripod is highly recommended to obtain the best results.

Once the input images are loaded, the interface presents you with a 100% preview of the result. The selected pixels from each input image are painted with a different color. You can then pan the result to inspect it.

When some objects were moving while you took the shots, there will appear "ghosts". You can use the toolbar to add or remove pixels from each image but the last one, until all the pixels that belong to a moving object only come from one of the input images. Usually, you will want to only remove pixels, starting with the first layer and then going down. Adding pixels to the first layers may result in burned areas appearing in the result image, so be careful. On the other hand, the pixels of the first layers contain less noise in the shadows. These operations can be undone and redone with the actions of the Edit menu.

Once the preview is satisfactory, the Save HDR option of the File menu generates the output DNG file. You can select the number of bits per sample (16, 24 or 32), the size of the embedded preview (full, half or no preview) and whether to save an image with the mask that was used to merge the input files. The number of bits per sample has an important impact in the output file size. As a rule of thumb, the default value of 16 bits will be enough most of the time. Empirical tests (thanks to DrSlony) show no apparent difference between 16- and 32-bit images, after merging 5 exposures with 2EV steps, despite strong manipulation of shadows/mid-tones/highlights. Nevertheless, if you see some unexpected quantization noise in the output image, you can try a 24-bit output. 32 bits will almost never be necessary, but it can be selected anyway.

The program can also be run without GUI, in batch mode. This is accomplished either by providing an output file name with the "-o" switch, or by generating an automatic one with the "-a" switch. Other switches control the output parameters, refer to the output of the "--help" switch. macOS users may need to change their current working directory to the one which contains the executable in order to run it in CLI mode.

## Licence
HDRMerge is released under the GNU General Public License v3.0.
See the file `LICENSE`.

## Contributing
Fork the project and send pull requests, or send patches by creating a new issue on our GitHub page:
https://github.com/jcelaya/hdrmerge/issues

## Reporting Bugs
Report bugs by creating a new issue on our GitHub page:
https://github.com/jcelaya/hdrmerge/issues

## Changelog:
- v0.5.0:
  - First Mac OS X build! Thanks to Philip Ries for his help.
  - Several bug fixes:
    - Fix dealing with images with non-ANSI file names.
    - Calculate response function with non-linear behavior.
    - Fix file locking issues by transfering Exif tags in memory.
    - Correctly calculate the response function of very dark images.
- v0.4.5:
  - Better compatibility with other programs, by producing a DNG file that maintains the original layout: frame and active area sizes, black and white levels, etc. *Note that, if you use RawTherapee, you need v4.1.23 or higher to open these files.
  - Batch mode in command line! Merge several sets of HDR images at once.
  - Creation of menu launchers and a Windows installer.
  - Support for CYGM and Fujifilm X-Trans sensors (experimental).
  - Several bug-fixes.
  - Improved accuracy and performance.
- v0.4.4:
  - Better support for more camera models.
  - Better rendering of the embedded preview image.
  - Change the edit brush radius with Alt+Mouse wheel.
  - Several bug fixes.
    - The original embedded preview is not included in the output anymore.
    - Fixed some glitches with the edit tools.
- v0.4.3:
  - Fix segmentation fault error painting the preview of some rotated images.
  - Fix DateTime tag in Windows hosts.
- v0.4.2:
  - Improved GUI:
    - A slider to control the brush radius.
    - A slider to control the preview exposure.
    - Movable toolbars.
    - Layer selector with color codes.
    - Improved brush visibility on different backgrounds.
    - Posibility of saving the output options.
  - First release with a Windows version, both 32- and 64-bit.
- v0.4.1:
  - Bugfixes release
- v0.4:
  - Great performance improvements with OpenMP.
  - Not depend anymore on DNG & XMP SDK! Windows and Mac version soon...
  - More robust MBT alignment.
  - More control on the logging output.
  - The user may disable alignment and/or cropping. This is most useful to obtain an image of the same size as the inputs. Some programs have this requirement to apply a flat-field image, for instance.
- v0.3: This is the first public version of HDRMerge
  - Supports most raw format supported by LibRaw (No foveon of Fuji formats for the moment).
  - Automatic alignment of small translations.
  - Automatic crop to the optimal size.
  - Automatic merge mask creation. The mask identifies the best source image for each pixel of the output.
  - Editable merge mask, to manually select pixels from specific source images.
  - Writes DNG files with 16, 24 and 32 bits per pixel.
  - Writes full, half or no preview to the output image.
  - Copies the EXIF data from the least exposed source image.

## Acknowledgments
I would like to thank all the people that have contributed ideas, critics and samples to improve HDRMerge. In particular, to the team of [RawTherapee](https://github.com/Beep6581/RawTherapee).

Also, HDRMerge implements or is based on the techniques described in the following works:
- Ward, G. (2003). Fast, robust image registration for compositing high dynamic range photographs from hand-held exposures. *Journal of graphics tools*, 8(2), 17-30.
- Guillermo Luijk, Zero Noise, <http://www.guillermoluijk.com/tutorial/zeronoise/index.html>
- Jens Mueller, dngconvert, <https://github.com/jmue/dngconvert>
- Jarosz, W. (2001). Fast image convolutions. In SIGGRAPH Workshop. Code from Ivan Kuckir, <http://blog.ivank.net/fastest-gaussian-blur.html>

There is also a community forum for discussions and connecting with other users (as well as other Free Software projects) at <https://discuss.pixls.us>, hosted by [PIXLS.US](https://pixls.us).

## Links
Website and documentation: http://jcelaya.github.io/hdrmerge/
Forum: https://discuss.pixls.us/c/software/hdrmerge
GitHub: https://github.com/jcelaya/hdrmerge
