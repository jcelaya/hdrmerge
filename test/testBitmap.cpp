#include "../Bitmap.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

BOOST_AUTO_TEST_CASE(bitmap_create) {
    Bitmap b(100, 200);
    b.set(20, 30);
    b.set(45, 81);
    b.set(99, 199);
    BOOST_CHECK(b.get(45, 81));
    BOOST_CHECK_EQUAL(b.count(), 3);
    b.reset(20, 30);
    BOOST_CHECK_EQUAL(b.count(), 2);
}

BOOST_AUTO_TEST_CASE(bitmap_shift) {
    Bitmap b(10, 10);
    b.set(2, 3);
    b.set(4, 8);
    b.set(9, 9);
    Bitmap bshifted1, bshifted2, bshifted3, bshifted4;
    bshifted1.shift(b, 3, 1);
    bshifted2.shift(b, -3, 1);
    bshifted3.shift(b, 1, -4);
    bshifted4.shift(b, -3, -4);
    BOOST_CHECK(bshifted1.get(5, 4));
    BOOST_CHECK(bshifted1.get(7, 9));
    BOOST_CHECK_EQUAL(bshifted1.count(), 2);
    BOOST_CHECK(bshifted2.get(1, 9));
    BOOST_CHECK_EQUAL(bshifted2.count(), 1);
    BOOST_CHECK(bshifted3.get(5, 4));
    BOOST_CHECK_EQUAL(bshifted3.count(), 1);
    BOOST_CHECK(bshifted4.get(1, 4));
    BOOST_CHECK(bshifted4.get(6, 5));
    BOOST_CHECK_EQUAL(bshifted4.count(), 2);
}

BOOST_AUTO_TEST_CASE(bitmap_xor) {
    Bitmap b1(10, 10), b2(10, 10);
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
    Bitmap b;
    b.mtb(data, 3, 7, 3);
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
    uint16_t data[] = { 1, 2, 3, 4, 3, 4, 5, 2, 3, 1, 4, 2, 5, 2, 3, 1, 2, 2, 2, 4, 5 };
    Bitmap b;
    b.exclusion(data, 3, 7, 3, 1);
    BOOST_CHECK_EQUAL(b.count(), 6);
    BOOST_CHECK(b.get(0, 0));
    BOOST_CHECK(b.get(0, 2));
    BOOST_CHECK(b.get(0, 3));
    BOOST_CHECK(b.get(0, 4));
    BOOST_CHECK(b.get(0, 5));
    BOOST_CHECK(b.get(2, 6));
}
