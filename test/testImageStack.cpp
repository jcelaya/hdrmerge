#include "../Image.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;

static const char * image1 = "test/sample1.dng";
static const char * image2 = "test/sample2.dng";

BOOST_AUTO_TEST_CASE( image_load ) {
    Image e1(image1);
    e1.dumpInfo();
    Image::LoadResult result = e1.load(nullptr);
    BOOST_REQUIRE_EQUAL(result, Image::LOAD_SUCCESS);
    e1.dumpInfo();
    Image e2(image2);
    result = e2.load(&e1);
    BOOST_REQUIRE_EQUAL(result, Image::LOAD_SUCCESS);
    e2.dumpInfo();
}
