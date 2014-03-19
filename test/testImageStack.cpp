#include "../ImageStack.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

static const char * image1 = "test/sample1.dng";
static const char * image2 = "test/sample2.dng";

BOOST_AUTO_TEST_CASE( image_load ) {
    Image e1(image1);
    BOOST_REQUIRE(!e1.loadFailed());
    Image e2(image2);
    BOOST_REQUIRE(!e2.loadFailed());
    BOOST_REQUIRE(!e2.isWrongFormat(e1));
    e1.alignWith(e2, 0.5, 1.0/4096);
    e2.alignWith(e1, 0.5, 1.0/4096);
    BOOST_CHECK_EQUAL(e1.getDeltaX(), -e2.getDeltaX());
    BOOST_CHECK_EQUAL(e1.getDeltaY(), -e2.getDeltaY());
}

BOOST_AUTO_TEST_CASE(stack_load) {
    ImageStack images;
    BOOST_CHECK_EQUAL(images.size(), 0);
    unique_ptr<Image> e1(new Image(image1)), e2(new Image(image2));
    BOOST_REQUIRE(!e1->loadFailed());
    BOOST_REQUIRE(!e2->loadFailed());
    BOOST_REQUIRE(images.addImage(e1));
    BOOST_CHECK(!e1.get());
    BOOST_CHECK(images.addImage(e2));
    BOOST_CHECK_EQUAL(images.size(), 2);
    BOOST_CHECK_EQUAL(images.getImage(0).getWidth(), images.getWidth());
    BOOST_CHECK_EQUAL(images.getImage(0).getHeight(), images.getHeight());
}
