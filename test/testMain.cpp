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

#define BOOST_TEST_MODULE hdrmerge
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../src/Log.hpp"
#include <iostream>

static int setLog() {
    hdrmerge::Log::setOutputStream(std::cout);
    hdrmerge::Log::setMinimumPriority(0);
    return 0;
}
static int foo = setLog();
