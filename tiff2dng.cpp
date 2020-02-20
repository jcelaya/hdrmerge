#include <iostream>

#include "src/DngFloatWriter.hpp"
#include "src/ImageIO.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>


void convert(std::string filename) {
    std::cout << "reading input file " << filename << std::endl;
    cv::Mat_<uint16_t> im = cv::imread(filename, cv::IMREAD_UNCHANGED);
    int const width = im.cols;
    int const height = im.rows;
    std::cout << "size: " << im.size() << std::endl;
    hdrmerge::Array2D<float> arr(width, height);
    std::cout << "hdrmerge::Array2D size: " << arr.size() << std::endl;
    for (int yy = 0; yy < height; ++yy) {
        for (int xx = 0; xx < width; ++xx) {
            arr(xx, yy) = double(im(yy, xx));
        }
    }
    cv::Mat preview;
    cv::cvtColor(im, preview, cv::COLOR_BayerBG2BGR);
    cv::imwrite(filename + "-preview.tif", preview);
    hdrmerge::DngFloatWriter writer;
    writer.setPreviewWidth(100); // This is important, otherwise
    hdrmerge::RawParameters params;
    params.height = params.rawHeight = height;
    params.width = params.rawWidth = width;
    params.fileName = filename.c_str();
    params.isoSpeed = 100;
    params.shutter = 100;
    params.aperture = 2.8;
    //params.FC.setPattern(2);

    std::cout << "Writing result" << std::endl;
    writer.write(std::move(arr), params, (filename + ".dng").c_str());
    std::cout << "done." << std::endl;
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " input files..." << std::endl;
    }


    for (int ii = 1; ii < argc; ++ii) {
        convert(argv[ii]);
    }
}
