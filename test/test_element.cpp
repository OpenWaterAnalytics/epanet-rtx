#include "test_main.h"
#include "Junction.h"
#include <iostream>

BOOST_AUTO_TEST_SUITE(Element)

BOOST_AUTO_TEST_CASE(ElementMetadataTest)
{
  RTX::Element::_sp e(new RTX::Junction("test element"));

  e->setMetadata(string("a_key_for_double"), 5.0);
  double value = std::get<double>(e->getMetadata("a_key_for_double"));
  BOOST_CHECK_CLOSE(value, 5.0, 0.0001);

  e->setMetadata(string("a_key_for_string"), string("A string"));
  BOOST_CHECK_EQUAL(std::get<std::string>(e->getMetadata("a_key_for_string")), "A string");
  
  RTX::TimeSeries::_sp ts(new RTX::TimeSeries());
  e->setMetadata(string("timeseries_key"), ts);
  BOOST_CHECK_EQUAL(std::get<RTX::TimeSeries::_sp>(e->getMetadata("timeseries_key")), ts);

  std::vector<std::string> keys = e->getMetadataKeys();
  BOOST_CHECK_EQUAL(keys.size(), 3);
}

BOOST_AUTO_TEST_CASE(ElementRemoveMetadataTest)
{
  RTX::Element::_sp e(new RTX::Junction("test element"));

  e->setMetadata(string("a_key_for_double"), 5.0);
  BOOST_CHECK_NO_THROW(e->removeMetadata("a_key_for_double"));
  BOOST_CHECK_THROW(e->getMetadata("a_key_for_double"), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()
