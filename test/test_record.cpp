#include "test_main.h"
#include "ConcreteDbRecords.h"
#include "Units.h"

using namespace RTX;
using namespace std;

////////////////////////
// record
BOOST_AUTO_TEST_SUITE(record)

BOOST_AUTO_TEST_CASE(record_basic) {
  
  const string recordName("test record");
  const string connection("local-file.db");
  
  // create demo record
  DbPointRecord::_sp record(new SqlitePointRecord);
  record->setName(recordName);
  record->setConnectionString(connection);

  record->registerAndGetIdentifierForSeriesWithUnits("test", Units::unitOfType("ft"));
  auto ids = record->identifiersAndUnits();
  BOOST_TEST(ids.hasIdentifierAndUnits("test", Units::unitOfType("ft")));
  BOOST_TEST(!ids.hasIdentifierAndUnits("test2", Units::unitOfType("gpm")));

  BOOST_CHECK_EQUAL(record->name(), recordName);
  BOOST_CHECK_EQUAL(record->connectionString(), connection);
}

BOOST_AUTO_TEST_SUITE_END()
// record
/////////////////////////
