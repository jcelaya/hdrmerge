#include <iostream>

#include "src/DngFloatWriter.hpp"
#include "src/ImageIO.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>


void convert(std::string filename) {
    std::cout << "reading input file " << filename << std::endl;
    cv::Mat_<uint16_t> im = cv::imread(filename, cv::IMREAD_UNCHANGED);
    double minval = 0;
    double maxval = 0;
    cv::minMaxIdx(im, &minval, &maxval);
    im *= 65535.0/maxval;
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
    preview.convertTo(preview, CV_8UC3, 1.0/256);
    cv::resize(preview, preview, cv::Size(), .5, .5);
    //cv::imwrite(filename + "-preview.tif", preview);
    cv::cvtColor(preview, preview, cv::COLOR_BGR2RGB);
    hdrmerge::RawParameters params((filename + ".dng").c_str());
    params.height = params.rawHeight = height;
    params.width = params.rawWidth = width;
    params.fileName = filename.c_str();
    params.isoSpeed = 100;
    params.shutter = .1;
    params.aperture = 2.8;
    params.colors = 3;
    params.black = 0;


    //QImage q_preview = hdrmerge::ImageIO::renderPreview(arr, params, 1.0);
    QImage q_preview = QImage(preview.data, preview.size().width, preview.size().height, QImage::Format_RGB888);
    std::cout << "q_preview width / height: " << q_preview.width() << " / " << q_preview.height() << std::endl;
    hdrmerge::DngFloatWriter writer;
    writer.setBitsPerSample(16);
    writer.setPreviewWidth(q_preview.width());
    writer.setPreview(q_preview);


    params.FC.setPattern(3031741620);

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
