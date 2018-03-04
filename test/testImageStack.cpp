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

#include <iostream>
#include <QDir>
#include "../src/ImageIO.hpp"
#include "SampleImage.hpp"
#include "../src/Log.hpp"
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

struct ImageIOFixture {
    ImageIO io;
    Image e1, e2, e3, e4;
};


BOOST_FIXTURE_TEST_CASE(image_load, ImageIOFixture) {
    RawParameters m1(image1), m2(image2), m3(sample1);
    e1 = io.loadRawImage(m1);
    BOOST_REQUIRE(e1.good());
    e2 = io.loadRawImage(m2);
    BOOST_REQUIRE(e2.good());
    BOOST_REQUIRE(m2.isSameFormat(m1));
    e3 = io.loadRawImage(m3);
    BOOST_CHECK(!e3.good());
}


BOOST_FIXTURE_TEST_CASE(image_align, ImageIOFixture) {
    SampleImage si1(sample1);
    SampleImage si2(sample2);
    SampleImage si3(sample3);
    SampleImage si4(sample4);
    e1 = Image(si1.begin(), si1.params);
    e2 = Image(si2.begin(), si2.params);
    e3 = Image(si3.begin(), si3.params);
    e4 = Image(si4.begin(), si4.params);
    BOOST_REQUIRE(e1.good());
    BOOST_REQUIRE(e2.good());
    BOOST_REQUIRE(e3.good());
    BOOST_REQUIRE(e4.good());
    e1.preScale(); e1.setSaturationThreshold(254);
    e2.preScale(); e2.setSaturationThreshold(254);
    e3.preScale(); e3.setSaturationThreshold(254);
    e4.preScale(); e4.setSaturationThreshold(254);
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
    // Align in chain, should remain aligned with e1
    e3.alignWith(e2);
    e4.alignWith(e3);
    e3.displace(e2.getDeltaX(), e2.getDeltaY());
    BOOST_CHECK_EQUAL(e3.getDeltaX(), 38);
    BOOST_CHECK_EQUAL(e3.getDeltaY(), 26);
    e4.displace(e3.getDeltaX(), e3.getDeltaY());
    BOOST_CHECK_EQUAL(e4.getDeltaX(), 32);
    BOOST_CHECK_EQUAL(e4.getDeltaY(), -4);
}

BOOST_AUTO_TEST_CASE(stack_load) {
    ImageStack images;
    BOOST_CHECK_EQUAL(images.size(), 0);
    RawParameters m1(image2), m2(image1);
    Image e1(ImageIO::loadRawImage(m1)), e2(ImageIO::loadRawImage(m2));
    BOOST_REQUIRE(e1.good());
    BOOST_REQUIRE(e2.good());
    images.addImage(std::move(e1));
    BOOST_CHECK_EQUAL(images.addImage(std::move(e2)), 0);
    BOOST_CHECK_EQUAL(images.size(), 2);
    BOOST_CHECK_EQUAL(images.getImage(0).getWidth(), images.getWidth());
    BOOST_CHECK_EQUAL(images.getImage(0).getHeight(), images.getHeight());
}

BOOST_AUTO_TEST_CASE(stack_align) {
    ImageStack images;
    SampleImage si1(sample1);
    SampleImage si2(sample2);
    SampleImage si3(sample3);
    SampleImage si4(sample4);
    Image e1(si1.begin(), si1.params),
        e2(si2.begin(), si2.params),
        e3(si3.begin(), si3.params),
        e4(si4.begin(), si4.params);
    BOOST_REQUIRE(e1.good());
    BOOST_REQUIRE(e2.good());
    BOOST_REQUIRE(e3.good());
    BOOST_REQUIRE(e4.good());
    e1.setSaturationThreshold(254);
    e2.setSaturationThreshold(254);
    e3.setSaturationThreshold(254);
    e4.setSaturationThreshold(254);
    images.addImage(std::move(e1));
    images.addImage(std::move(e2));
    images.addImage(std::move(e3));
    images.addImage(std::move(e4));
    images.setFlip(0);
    measureTime("Align images total", [&] () {images.align();});
    images.crop();
    Image & e1ref = images.getImage(1), & e2ref = images.getImage(3),
        & e3ref = images.getImage(2), & e4ref = images.getImage(0);
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
    Image e1, e2, e3;
    RawParameters m1(image1), m2(image2), m3(image3);
    measureTime("Load images", [&] () {
        e1 = ImageIO::loadRawImage(m1);
        e2 = ImageIO::loadRawImage(m2);
        e3 = ImageIO::loadRawImage(m3);
    });
    BOOST_REQUIRE(e1.good());
    BOOST_REQUIRE(e2.good());
    BOOST_REQUIRE(e3.good());

    images.addImage(std::move(e1));
    images.addImage(std::move(e2));
    images.addImage(std::move(e3));
    Image & e1ref = images.getImage(0), & e2ref = images.getImage(1), & e3ref = images.getImage(2);
    images.setFlip(m1.flip);
    images.calculateSaturationLevel(m1);
    measureTime("Align images", [&] () {
        images.align();
    });

    measureTime("Compute response functions", [&] () {
        images.computeResponseFunctions();
    });
    double metaImmExp = 1.0 / (1 << (int)(m1.logExp() - m2.logExp()));
    double dataImmExp = e1ref.getRelativeExposure() / e2ref.getRelativeExposure();
    cerr << "Relative exposure from data: " << dataImmExp << endl;
    cerr << "Relative exposure from metadata: " << metaImmExp << endl;
}


struct NullProgressIndicator : public ProgressIndicator {
    virtual void advance(int percent, const char * message, const char * arg) {}
};


BOOST_AUTO_TEST_CASE(output_filename) {
    ImageIO io;
    LoadOptions lo;
    lo.align = lo.crop = false;
    NullProgressIndicator npi;
    lo.fileNames.push_back(image1);
    BOOST_REQUIRE_EQUAL(io.load(lo, npi), 2);
    string pwd = QDir::currentPath().toLocal8Bit().constData();
    string oneFile = io.buildOutputFileName().toLocal8Bit().constData();
    BOOST_CHECK_EQUAL(oneFile, pwd + "/test/sample1.dng");
    lo.fileNames.push_back(image2);
    lo.fileNames.push_back(image3);
    BOOST_REQUIRE_EQUAL(io.load(lo, npi), 6);
    string threeFile = io.buildOutputFileName().toLocal8Bit().constData();
    BOOST_CHECK_EQUAL(threeFile, pwd + "/test/sample1-3.dng");
}
