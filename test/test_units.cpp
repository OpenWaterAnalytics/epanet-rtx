#include "test_main.h"
#include "Units.h"

using namespace RTX;
using namespace std;

////////////////////////
// units
BOOST_AUTO_TEST_SUITE(units)

BOOST_AUTO_TEST_CASE(units_strings) {
  for (auto upr : Units::unitStringMap) {
    string ustr = upr.first;
    Units u = upr.second;
    string fromU = u.to_string();
    Units unitsFromRawStr = Units::unitOfType(u.rawUnitString());
    string fromRaw = unitsFromRawStr.to_string();
    BOOST_CHECK_EQUAL(ustr, fromU);
    BOOST_CHECK_EQUAL(ustr, fromRaw);
    bool ok = (u == unitsFromRawStr);
    BOOST_TEST(ok, "Units objects not equal");
  }
}

BOOST_AUTO_TEST_SUITE_END()
// units
/////////////////////////

