//
//  main.cpp
//  data_access_profiling
//

#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/timer/timer.hpp>
#include <boost/foreach.hpp>

#include <boost/timer/timer.hpp>

#include "TimeSeries.h"
#include "OpcPointRecord.h"
#include <boost/thread.hpp>

using namespace std;
using namespace RTX;



int main(int argc, const char * argv[])
{
  
  
  OpcPointRecord::_sp opc(new OpcPointRecord());
  
  opc->setConnectionString("opc.tcp://10.0.1.14:4096");
    
  opc->dbConnect();
  
  map<string,Units> ids = opc->identifiersAndUnits();
  
  for (auto item : ids) {
    cout << item.first << endl;
  }
  
  
  while (true) {
    
    vector<Point> points = opc->pointsInRange("[simulated]_Meta:Ramp/Ramp1", time(NULL)-5, time(NULL)+5);
    
    for (auto p : points) {
      cout << p << endl;
    }
    
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  }
  
  
  
  
}
