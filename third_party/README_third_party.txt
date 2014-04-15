You need Adobe DNG SDK v1.4 and XMP SDK CC 2013.06 to build HDRMerge. The
DNG SDK is included, but you can get them from:

[1] http://www.adobe.com/devnet/xmp.html
[2] https://www.adobe.com/support/downloads/dng/dng_sdk.html

To build the XMP SDK:
1 - Unzip the sdk in the third_party directory.
2 - Rename the XMP SDK directory as xmpsdk.
3 - Cd into xmpsdk/third_party and follow the instructions to download
    libexpat and zlib.
4 - Cd into xmpsdk/build and run 'make StaticAll'. Known problems:
    - Depending on your version of gcc, it may give lots of warnings, just
      ignore them.
    - It may fail at shared/SharedConfig_Common.cmake, comment out the lines
      failing.
    - Also, XMPFiles/source/NativeMetadataSupport/ValueObject.h should include
      string.h
5 - Move or symlink public/libraries/*/release/staticXMP{Core,Files}.ar to
    public/libraries/libXMP{Core,Files}.a
