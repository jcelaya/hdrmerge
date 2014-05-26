---
layout: post
image: using-hdrmerge.jpg
title: Using HDRMerge
categories: documentation
---
# Usage:

Source images can be loaded from the Open option in the File menu, or passed as arguments in the command line. They must be made with the same camera. After loading them, HDRMerge will correct small misalignments by translation. So, if your camera allows it, you can take the shots with bracketing in burst mode. I have successfully done this just holding the camera with my hands, but using a tripod is highly recommended to obtain the best results.

Once the input images are loaded, the interface presents you with a 100% preview of the result. The selected pixels from each input image are painted with a different color. You can then pan the result to inspect it.

When some objects were moving while you took the shots, there will appear "ghosts". You can use the bottom toolbar to add or remove pixels from each image but the last one, until all the pixels that belong to a moving object only come from one of the input images. Usually, you will want to only remove pixels, starting with the first layer and then going down. Adding pixels to the first layers may result in burned areas appearing in the result image, so be careful. On the other hand, the pixels of the first layers contain less noise in the shadows. These operations can be undone and redone with the actions of the Edit menu.

Once the preview is satisfactory, the Save HDR option of the File menu generates the output DNG file. You can select the number of bits per sample (16, 24 or 32), the size of the embedded preview (full, half or no preview) and whether to save an image with the mask that was used to merge the input files. The number of bits per sample has an important impact in the output file size. As a rule of thumb, the default value of 16 bits will be enough most of the time. Empirical tests (thanks to DrSlony) show no apparent difference between 16- and 32-bit images, after merging 5 exposures with 2EV steps, despite strong manipulation of shadows/mid-tones/highlights. Nevertheless, if you see some unexpected noise in the shadows of the output image, you can try a 24-bit output. 32 bits will almost never be necessary, but it can be selected anyway.

The program can also be run without GUI, in batch mode. This is accomplished either by providing an output file name with the "-o" switch, or by generating an automatic one with the "-a" switch. Other switches control the output parameters, refer to the output of the "--help" switch.

