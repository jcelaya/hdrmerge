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

#include "../Bitmap.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

BOOST_AUTO_TEST_CASE(bitmap_create) {
    Bitmap b(100, 200);
    b.reset();
    b.set(20, 30);
    b.set(45, 81);
    b.set(99, 199);
    BOOST_CHECK(b.get(45, 81));
    BOOST_CHECK_EQUAL(b.count(), 3);
    b.reset(20, 30);
    BOOST_CHECK_EQUAL(b.count(), 2);
}

static bool checkZeroArea(const Bitmap & b, int i0, int i1, int j0, int j1) {
    for (int i = i0; i < i1; ++i) {
        for (int j = j0; j < j1; ++j) {
            if (b.get(i, j)) return false;
        }
    }
    return true;
}

BOOST_AUTO_TEST_CASE(bitmap_shift) {
    Bitmap b(100, 100);
    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
            b.set(i, j);
        }
    }
    b.reset(49, 49); b.reset(49, 50); b.reset(50, 49); b.reset(50, 50);
    b.dumpFile();
    Bitmap s(100, 100);
    s.shift(b, 35, 35);
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 0, 35, 0, 100), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 0, 100, 0, 35), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 84, 86, 84, 86), s.dumpInfo());
    BOOST_CHECK_MESSAGE(s.count() == 4221, s.dumpInfo());
    s.shift(b, -35, 35);
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 65, 100, 0, 100), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 0, 100, 0, 35), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 14, 16, 84, 86), s.dumpInfo());
    BOOST_CHECK_MESSAGE(s.count() == 4221, s.dumpInfo());
    s.shift(b, 35, -35);
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 0, 35, 0, 100), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 0, 100, 65, 100), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 84, 86, 14, 16), s.dumpInfo());
    BOOST_CHECK_MESSAGE(s.count() == 4221, s.dumpInfo());
    s.shift(b, -35, -35);
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 65, 100, 0, 100), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 0, 100, 65, 100), s.dumpInfo());
    BOOST_CHECK_MESSAGE(checkZeroArea(s, 14, 16, 14, 16), s.dumpInfo());
    BOOST_CHECK_MESSAGE(s.count() == 4221, s.dumpInfo());
}

BOOST_AUTO_TEST_CASE(bitmap_xor) {
    Bitmap b1(10, 10), b2(10, 10);
    b1.reset();
    b2.reset();
    for (int i : { 1, 3, 4, 7, 8 }) {
        b1.set(i, 3);
        b2.set(i, 3);
    }
    b1.set(1, 5);
    b1.set(3, 7);
    b2.set(6, 6);
    b2.bitwiseXor(b1);
    BOOST_CHECK_EQUAL(b2.count(), 3);
    BOOST_CHECK(b2.get(1, 5));
    BOOST_CHECK(b2.get(3, 7));
    BOOST_CHECK(b2.get(6, 6));
}

BOOST_AUTO_TEST_CASE(bitmap_and) {
    Bitmap b1(10, 10), b2(10, 10);
    b1.reset();
    b2.reset();
    for (int i : { 1, 3, 4, 7, 8 }) {
        b1.set(i, 3);
        b2.set(i, 3);
    }
    b1.set(1, 5);
    b1.set(3, 7);
    b2.set(6, 6);
    b2.bitwiseAnd(b1);
    BOOST_CHECK_EQUAL(b2.count(), 5);
    for (int i : { 1, 3, 4, 7, 8 }) {
        BOOST_CHECK(b2.get(i, 3));
    }
}

BOOST_AUTO_TEST_CASE(bitmap_mtb) {
    uint16_t data[] = { 1, 2, 3, 4, 3, 4, 5, 2, 3, 1, 4, 2, 5, 2, 3, 1, 2, 2, 2, 4, 5 };
    Bitmap b(3, 7);
    b.mtb(data, 3);
    BOOST_CHECK_EQUAL(b.count(), 7);
    BOOST_CHECK(b.get(0, 1));
    BOOST_CHECK(b.get(2, 1));
    BOOST_CHECK(b.get(0, 2));
    BOOST_CHECK(b.get(1, 3));
    BOOST_CHECK(b.get(0, 4));
    BOOST_CHECK(b.get(1, 6));
    BOOST_CHECK(b.get(2, 6));
}

BOOST_AUTO_TEST_CASE(bitmap_exclusion) {
    uint16_t data[] = { 1, 2, 3, 4, 3, 4, 5, 2, 3, 1, 4, 3, 5, 2, 3, 1, 3, 4, 2, 4, 5 };
    Bitmap b(3, 7);
    b.exclusion(data, 3, 1);
    BOOST_CHECK_EQUAL(b.count(), 10);
    BOOST_CHECK(b.get(0, 0));
    BOOST_CHECK(b.get(1, 0));
    BOOST_CHECK(b.get(0, 2));
    BOOST_CHECK(b.get(1, 2));
    BOOST_CHECK(b.get(0, 3));
    BOOST_CHECK(b.get(0, 4));
    BOOST_CHECK(b.get(1, 4));
    BOOST_CHECK(b.get(0, 5));
    BOOST_CHECK(b.get(0, 6));
    BOOST_CHECK(b.get(2, 6));
}
