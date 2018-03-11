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

#include "../src/Histogram.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

namespace hdrmerge {

struct HistogramFixture {
    Histogram h;
    uint16_t values[14];

    HistogramFixture() : values{8, 3, 6, 4, 5, 2, 1, 7, 9, 3, 2, 7, 9, 9} {}
    ~HistogramFixture() {
        BOOST_CHECK_EQUAL(h.getNumSamples(), 14);
        BOOST_CHECK_EQUAL(h.getPercentile(0.5), 5);
        BOOST_CHECK_EQUAL(h.getPercentile(0.75), 7);
        BOOST_CHECK_EQUAL(h.getPercentile(0.2), 2);
        BOOST_CHECK_EQUAL(h.getFraction(2), 3.0/14.0);
    }
};


BOOST_FIXTURE_TEST_CASE(Histogram_addValue, HistogramFixture) {
    for (uint16_t i : values) {
        h.addValue(i);
    }
}


BOOST_FIXTURE_TEST_CASE(Histogram_ctr, HistogramFixture) {
    h = Histogram(values, values + 14);
}

} // namespace hdrmerge
