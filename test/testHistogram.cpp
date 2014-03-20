#include "../Histogram.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

BOOST_AUTO_TEST_CASE(testHistogram) {
    Histogram h;
    for (uint16_t i : {8, 3, 6, 4, 5, 2, 1, 7, 9, 3, 2, 7, 9, 9}) {
        h.addValue(i);
    }
    BOOST_CHECK_EQUAL(h.getMedian(0.5), 5);
    BOOST_CHECK_EQUAL(h.getMedian(0.75), 7);
    BOOST_CHECK_EQUAL(h.getMedian(0.2), 2);
}
