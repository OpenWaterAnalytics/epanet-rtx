#include <iostream>
#include <time.h>
#include <boost/foreach.hpp>

#include "SqliteProjectFile.h"

using namespace std;
using namespace RTX;


int main (int argc, const char * argv[])
{
  
  
  SqliteProjectFile::sharedPointer proj( new SqliteProjectFile() );
  
  proj->loadProjectFile("testdb.rtx");
  
  RTX_LIST<TimeSeries::sharedPointer> tsList = proj->timeSeries();
  
  
  BOOST_FOREACH(TimeSeries::sharedPointer ts, tsList) {
    cout << *ts << endl;
  }

}