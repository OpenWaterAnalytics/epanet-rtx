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


using namespace std;
using namespace RTX;



int main(int argc, const char * argv[])
{
  
  
  OpcPointRecord::_sp opc(new OpcPointRecord());
  
  opc->setConnectionString("opc.tcp://10.0.1.14:4096/iaopcua/None");
    
  opc->dbConnect();
  
  
}
