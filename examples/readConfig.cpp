#include <iostream>
#include <time.h>
#include <boost/foreach.hpp>
#include <iomanip>

#include "SqliteProjectFile.h"

using namespace std;
using namespace RTX;


int main (int argc, const char * argv[])
{
  int intQual = 332;
  string tStr = "2014-05-06 12:32:02";
  double v = 223.4455656632;
  
  std::bitset<16> bits(intQual);
  cout << "#" << setw(20) << tStr << " | " << setw(10) << v << " | " << "Q: " << setw(6) << intQual << " | " << setw(16) << bits << endl;
  
  cout << endl;
  
  return 0;
  
  
  
  SqliteProjectFile::sharedPointer proj( new SqliteProjectFile() );
  
  proj->loadProjectFile("testdb.rtx");
  
  RTX_LIST<TimeSeries::sharedPointer> tsList = proj->timeSeries();
  
  
  BOOST_FOREACH(TimeSeries::sharedPointer ts, tsList) {
    cout << *ts << endl;
  }

}