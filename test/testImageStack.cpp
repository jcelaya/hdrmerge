/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../ImageStack.hpp"
#include "SampleImage.hpp"
#include "time.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/config/no_tr1/complex.hpp>
using namespace hdrmerge;
using namespace std;


static const char * image1 = "test/sample1.dng";
static const char * image2 = "test/sample2.dng";
static const char * image3 = "test/sample3.dng";
// Sample images with deviation
static const char * sample1 = "test/sample1.png"; // (0, 0)
static const char * sample2 = "test/sample2.png"; // (20, 32)
static const char * sample3 = "test/sample3.png"; // (38, 26)
static const char * sample4 = "test/sample4.png"; // (34, -4)


BOOST_AUTO_TEST_CASE(image_load) {
    Image e1(image1);
    BOOST_REQUIRE(e1.good());
    Image e2(image2);
    BOOST_REQUIRE(e2.good());
    BOOST_REQUIRE(e2.isSameFormat(e1));
    Image e3(sample1);
    BOOST_CHECK(!e3.good());
}


// BOOST_AUTO_TEST_CASE(image_scale) {
//     SampleImage si1(sample1);
//     Image im(si1.pixelData, si1.md);
//     BOOST_REQUIRE(im.good());
//     char filename[] = "scaledx.png";
//     size_t w = im.getWidth(), h = im.getHeight();
//     for (int i = 0; i < Image::scaleSteps; ++i) {
//         filename[6] = '0' + i;
//         save(filename, im.getPixels(i), w, h);
//         w >>= 1;
//         h >>= 1;
//     }
// }

BOOST_AUTO_TEST_CASE(image_align) {
    SampleImage si1(sample1);
    SampleImage si2(sample2);
    SampleImage si3(sample3);
    SampleImage si4(sample4);
    Image e1(si1.pixelData, si1.md);
    Image e2(si2.pixelData, si2.md);
    Image e3(si3.pixelData, si3.md);
    Image e4(si4.pixelData, si4.md);
    e1.preAlignSetup(0.5, 1.0/64);
    e2.preAlignSetup(0.5, 1.0/64);
    e3.preAlignSetup(0.5, 1.0/64);
    e4.preAlignSetup(0.5, 1.0/64);
    BOOST_REQUIRE(e1.good());
    BOOST_REQUIRE(e2.good());
    BOOST_REQUIRE(e3.good());
    BOOST_REQUIRE(e4.good());
    e2.alignWith(e1);
    e3.alignWith(e1);
    e4.alignWith(e1);
    BOOST_CHECK_EQUAL(e1.getDeltaX(), 0);
    BOOST_CHECK_EQUAL(e1.getDeltaY(), 0);
    BOOST_CHECK_EQUAL(e2.getDeltaX(), 20);
    BOOST_CHECK_EQUAL(e2.getDeltaY(), 32);
    BOOST_CHECK_EQUAL(e3.getDeltaX(), 38);
    BOOST_CHECK_EQUAL(e3.getDeltaY(), 26);
    BOOST_CHECK_EQUAL(e4.getDeltaX(), 32);
    BOOST_CHECK_EQUAL(e4.getDeltaY(), -4);
    e3.alignWith(e2);
    e4.alignWith(e3);
    BOOST_CHECK_EQUAL(e3.getDeltaX(), 18);
    BOOST_CHECK_EQUAL(e3.getDeltaY(), -6);
    BOOST_CHECK_EQUAL(e4.getDeltaX(), -6);
    BOOST_CHECK_EQUAL(e4.getDeltaY(), -30);
}

BOOST_AUTO_TEST_CASE(stack_load) {
    ImageStack images;
    BOOST_CHECK_EQUAL(images.size(), 0);
    unique_ptr<Image> e1(new Image(image2)), e2(new Image(image1));
    Image & e1ref = *e1, & e2ref = *e2;
    BOOST_REQUIRE(e1->good());
    BOOST_REQUIRE(e2->good());
    BOOST_REQUIRE(images.addImage(e1));
    BOOST_CHECK(!e1.get());
    BOOST_CHECK(images.addImage(e2));
    BOOST_CHECK_EQUAL(images.size(), 2);
    BOOST_CHECK_EQUAL(&images.getImage(0), &e2ref);
    BOOST_CHECK_EQUAL(images.getImage(0).getWidth(), images.getWidth());
    BOOST_CHECK_EQUAL(images.getImage(0).getHeight(), images.getHeight());
}

BOOST_AUTO_TEST_CASE(stack_align) {
    ImageStack images;
    SampleImage si1(sample1);
    SampleImage si2(sample2);
    SampleImage si3(sample3);
    SampleImage si4(sample4);
    unique_ptr<Image> e1(new Image(si1.pixelData, si1.md)),
        e2(new Image(si2.pixelData, si2.md)),
        e3(new Image(si3.pixelData, si3.md)),
        e4(new Image(si4.pixelData, si4.md));
    Image & e1ref = *e1, & e2ref = *e2, & e3ref = *e3, & e4ref = *e4;
    BOOST_REQUIRE(e1->good());
    BOOST_REQUIRE(e2->good());
    BOOST_REQUIRE(e3->good());
    BOOST_REQUIRE(e4->good());
    BOOST_REQUIRE(images.addImage(e1));
    BOOST_REQUIRE(images.addImage(e2));
    BOOST_REQUIRE(images.addImage(e3));
    BOOST_REQUIRE(images.addImage(e4));
    double alignTime = measureTime([&] () {images.align();});
    cerr << "Images aligned in " << alignTime << " seconds." << endl;
    BOOST_CHECK_EQUAL(images.getWidth(), 962);
    BOOST_CHECK_EQUAL(images.getHeight(), 564);
    BOOST_CHECK_EQUAL(e1ref.getDeltaX(), -38);
    BOOST_CHECK_EQUAL(e1ref.getDeltaY(), -32);
    BOOST_CHECK_EQUAL(e2ref.getDeltaX(), -18);
    BOOST_CHECK_EQUAL(e2ref.getDeltaY(), 0);
    BOOST_CHECK_EQUAL(e3ref.getDeltaX(), 0);
    BOOST_CHECK_EQUAL(e3ref.getDeltaY(), -6);
    BOOST_CHECK_EQUAL(e4ref.getDeltaX(), -6);
    BOOST_CHECK_EQUAL(e4ref.getDeltaY(), -36);
}


BOOST_AUTO_TEST_CASE(auto_exposure) {
    ImageStack images;
    unique_ptr<Image> e1, e2, e3;
    double time = measureTime([&] () {
        e1.reset(new Image(image1));
        e2.reset(new Image(image2));
        e3.reset(new Image(image3));
    });
    cerr << "Images loaded in " << time << " seconds." << endl;
    BOOST_REQUIRE(e1->good());
    BOOST_REQUIRE(e2->good());
    BOOST_REQUIRE(e3->good());

    images.addImage(e1);
    images.addImage(e2);
    images.addImage(e3);
    Image & e1ref = images.getImage(0), & e2ref = images.getImage(1), & e3ref = images.getImage(2);
    time = measureTime([&] () {
        images.align();
    });
    cerr << "Images aligned in " << time << " seconds." << endl;

    time = measureTime([&] () {
        images.computeRelExposures();
    });
    double metaImmExp = 1.0 / (1 << (int)(e1ref.getMetaData().logExp() - e2ref.getMetaData().logExp()));
    double dataImmExp = e1ref.getRelativeExposure() / e2ref.getRelativeExposure();
    cerr << "Relative exposure from data: " << dataImmExp << " in " << time << " seconds." << endl;
    cerr << "Relative exposure from metadata: " << metaImmExp << endl;
}


BOOST_AUTO_TEST_CASE(output_filename) {
    ImageStack images;
    unique_ptr<Image> e1, e2, e3;
    e1.reset(new Image(image1));
    e2.reset(new Image(image2));
    e3.reset(new Image(image3));
    BOOST_REQUIRE(e1->good());
    BOOST_REQUIRE(e2->good());
    BOOST_REQUIRE(e3->good());

    images.addImage(e1);
    string oneFile = images.buildOutputFileName();
    BOOST_CHECK_EQUAL(oneFile, "test/sample1");
    images.addImage(e2);
    images.addImage(e3);
    string threeFile = images.buildOutputFileName();
    BOOST_CHECK_EQUAL(threeFile, "test/sample1-3");
}
