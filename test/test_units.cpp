#include "test_main.h"
#include "Units.h"

using namespace RTX;
using namespace std;

////////////////////////
// units
BOOST_AUTO_TEST_SUITE(units)

BOOST_AUTO_TEST_CASE(units_strings) {
  for (auto upr : Units::unitStrings) {
    string ustr = upr.first;
    Units u = upr.second;
    string fromU = u.to_string();
    Units unitsFromRawStr = Units::unitOfType(u.rawUnitString());
    string fromRaw = unitsFromRawStr.to_string();
    BOOST_TEST((unitsFromRawStr.isSameDimensionAs(u) || unitsFromRawStr.isInvalid()));
    BOOST_CHECK_CLOSE(unitsFromRawStr.conversion(), u.conversion(), 0.000001);
  }
}

BOOST_AUTO_TEST_CASE(units_convert_nan) {
  
  double n_a_n = nan("");
  double maybe_nan = Units::convertValue(n_a_n, RTX_GALLON_PER_MINUTE, RTX_GALLON_PER_DAY);
  BOOST_TEST(isnan(maybe_nan), "not-a-number is a number");
  
}

BOOST_AUTO_TEST_SUITE_END()
// units
/////////////////////////

