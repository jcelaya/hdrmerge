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

#include "../src/Array2D.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;


struct Array2DFixtureEmpty {
    Array2D<float> a;
};


BOOST_FIXTURE_TEST_CASE(array2d_create, Array2DFixtureEmpty) {
    BOOST_CHECK_EQUAL(a.getWidth(), 0);
    BOOST_CHECK_EQUAL(a.getHeight(), 0);
    BOOST_CHECK_EQUAL(a.getDeltaX(), 0);
    BOOST_CHECK_EQUAL(a.getDeltaY(), 0);
}


BOOST_FIXTURE_TEST_CASE(array2d_resize, Array2DFixtureEmpty) {
    a.resize(100, 10);
    BOOST_CHECK_EQUAL(a.getWidth(), 100);
    BOOST_CHECK_EQUAL(a.getHeight(), 10);
    BOOST_CHECK_EQUAL(a.getDeltaX(), 0);
    BOOST_CHECK_EQUAL(a.getDeltaY(), 0);
}


struct Array2DFixture100x10 {
    Array2D<float> a;
    Array2DFixture100x10() : a(100, 10) {}
};


BOOST_FIXTURE_TEST_CASE(array2d_create_with_size, Array2DFixture100x10) {
    BOOST_CHECK_EQUAL(a.getWidth(), 100);
    BOOST_CHECK_EQUAL(a.getHeight(), 10);
    BOOST_CHECK_EQUAL(a.getDeltaX(), 0);
    BOOST_CHECK_EQUAL(a.getDeltaY(), 0);
}


BOOST_FIXTURE_TEST_CASE(array2d_displace, Array2DFixture100x10) {
    a.displace(5, -8);
    BOOST_CHECK_EQUAL(a.getWidth(), 100);
    BOOST_CHECK_EQUAL(a.getHeight(), 10);
    BOOST_CHECK_EQUAL(a.getDeltaX(), 5);
    BOOST_CHECK_EQUAL(a.getDeltaY(), -8);
    BOOST_CHECK(!a.contains(4, -4));
    BOOST_CHECK(!a.contains(4, 1));
    BOOST_CHECK(!a.contains(20, 5));
    BOOST_CHECK(!a.contains(103, 5));
    BOOST_CHECK(a.contains(103, 1));
    BOOST_CHECK(a.contains(20, -4));
}


BOOST_FIXTURE_TEST_CASE(array2d_access, Array2DFixture100x10) {
    a[0] = -1.0;
    BOOST_CHECK_EQUAL(a[0], -1.0);
    a(2, 3) = 3.5;
    BOOST_CHECK_EQUAL(a(2, 3), 3.5);
    a.displace(-2, -3);
    BOOST_CHECK_EQUAL(a[0], -1.0);
    BOOST_CHECK_NE(a(2, 3), 3.5);
    BOOST_CHECK_EQUAL(a(0, 0), 3.5);
}


BOOST_FIXTURE_TEST_CASE(array2d_copy, Array2DFixture100x10) {
    a[0] = -1.0;
    a(2, 3) = 3.5;
    a.displace(-2, -3);
    Array2D<double> b(a);
    BOOST_CHECK_EQUAL(b[0], -1.0);
    BOOST_CHECK_NE(b(2, 3), 3.5);
    BOOST_CHECK_EQUAL(b(0, 0), 3.5);
}
