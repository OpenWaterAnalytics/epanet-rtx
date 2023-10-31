#include "test_main.h"
#include "Junction.h"

BOOST_AUTO_TEST_SUITE(Element)

BOOST_AUTO_TEST_CASE(ElementMetadataTest)
{
  RTX::Element::_sp e(new RTX::Junction("test element"));

  e->setMetadataValue(string("a_key_for_double"), 5.0);
  double value = std::get<double>(e->getMetadataValue("a_key_for_double"));
  BOOST_CHECK_CLOSE(value, 5.0, 0.0001);

  e->setMetadataValue(string("a_key_for_string"), string("A string"));
  BOOST_CHECK_EQUAL(std::get<std::string>(e->getMetadataValue("a_key_for_string")), "A string");
  
  RTX::TimeSeries::_sp ts(new RTX::TimeSeries());
  e->setMetadataValue(string("timeseries_key"), ts);
  BOOST_CHECK_EQUAL(std::get<RTX::TimeSeries::_sp>(e->getMetadataValue("timeseries_key")), ts);
}

BOOST_AUTO_TEST_CASE(ElementRemoveMetadataTest)
{
  RTX::Element::_sp e(new RTX::Junction("test element"));

  e->setMetadataValue(string("a_key_for_double"), 5.0);
  BOOST_CHECK_NO_THROW(e->removeMetadataValue("a_key_for_double"));
  BOOST_CHECK_THROW(e->getMetadataValue("a_key_for_double"), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()
