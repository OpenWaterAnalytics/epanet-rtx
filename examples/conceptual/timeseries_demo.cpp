//
//  timeseries_demo.cpp
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include <iostream>
#include <stdlib.h>

#include <boost/foreach.hpp>
#include <boost/random.hpp>

#include "Point.h"
#include "TimeSeries.h"
#include "Resampler.h"
#include "MovingAverage.h"
#include "FirstDerivative.h"
#include "SineTimeSeries.h"
#include "ConstantTimeSeries.h"
#include "AggregatorTimeSeries.h"
#include "CsvPointRecord.h"
#include "GainTimeSeries.h"
#include "FailoverTimeSeries.h"
#include "TimeOffsetTimeSeries.h"
#include "WarpingTimeSeries.h"
#include "ForecastTimeSeries.h"

#include "OutlierExclusionTimeSeries.h"

#include "BufferPointRecord.h"
#include "MysqlPointRecord.h"

//#include "SqliteProjectFile.h"

#include "SqlitePointRecord.h"

#include "PythonInterpreter.h"

using namespace RTX;
using namespace std;

void printPoints(vector<Point> pointVector);

int main(int argc, const char * argv[])
{
  
  SineTimeSeries::sharedPointer sineWave(new SineTimeSeries());
  
  sineWave->setName("sine");
  
  ForecastTimeSeries::sharedPointer pythonTestTs(new ForecastTimeSeries());
  
  pythonTestTs->setSource(sineWave);
  
  vector<Point> testPoints = pythonTestTs->points(1000, 3000);
  
  printPoints(testPoints);
  
  
  vector<Point> range;
  
  
  
  
  return 0;
}


void printPoints(vector<Point> pointVector) {
  BOOST_FOREACH(Point thePoint, pointVector) {
    cout << thePoint << endl;
  }
}

