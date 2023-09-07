#include "test_main.h"
#include "Element.h"

BOOST_AUTO_TEST_SUITE(Element)

BOOST_AUTO_TEST_CASE(ElementMetadataTest)
{
    Rtx::Element element;

    element.setMetadata("a_key_for_double", 5.0);
    BOOST_CHECK_CLOSE(element.getMetadata("a_key_for_double"), 5.0, 0.0001);

    element.setMetadata("a_key_for_float", 2.33f);
    BOOST_CHECK_CLOSE(element.getMetadata("a_key_for_float"), 2.33f, 0.0001);

    element.setMetadata("a_key_for_string", "A string");
    BOOST_CHECK_EQUAL(element.getMetadata("a_key_for_string"), "A string");
}

BOOST_AUTO_TEST_CASE(ElementRemoveMetadataTest)
{
    Rtx::Element element;

    element.setMetadata("a_key_for_double", 5.0);
    BOOST_CHECK_NO_THROW(element.removeMetadata("a_key_for_double"));
    BOOST_CHECK_THROW(element.getMetadata("a_key_for_double"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()