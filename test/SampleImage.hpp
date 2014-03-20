#include "../MetaData.hpp"
#include <OpenImageIO/imageio.h>

struct SampleImage {
    uint16_t * pixelData;
    hdrmerge::MetaData md;
    SampleImage(const char * f) : pixelData(nullptr) {
        OpenImageIO::ImageInput *in = OpenImageIO::ImageInput::create(f);
        if (! in)
            return;
        OpenImageIO::ImageSpec spec;
        in->open(f, spec);
        md.width = spec.width;
        md.height = spec.height;
        md.max = 65535;
        pixelData = new uint16_t[md.width*md.height*2];
        in->read_image(OpenImageIO::TypeDesc::UINT16, pixelData);
        in->close();
        delete in;
    }
    ~SampleImage() {
        if (pixelData)
            delete[] pixelData;
    }
};

void save(const char * f, const uint16_t * pixels, size_t w, size_t h) {
    OpenImageIO::ImageOutput *out = OpenImageIO::ImageOutput::create (f);
    if (! out)
        return;
    OpenImageIO::ImageSpec spec (w, h, 1, OpenImageIO::TypeDesc::UINT8);
    out->open (f, spec);
    out->write_image (OpenImageIO::TypeDesc::UINT16, pixels);
    out->close ();
    delete out;

}
