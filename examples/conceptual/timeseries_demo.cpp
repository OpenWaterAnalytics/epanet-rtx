//
//  timeseries_demo.cpp
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include <iostream>
#include <stdlib.h>

//#include <boost/random.hpp>

#include "Point.h"
#include "TimeSeries.h"
//#include "Resampler.h"
#include "MovingAverage.h"
#include "FirstDerivative.h"
#include "SineTimeSeries.h"
#include "ConstantTimeSeries.h"
#include "AggregatorTimeSeries.h"
//#include "CsvPointRecord.h"
#include "GainTimeSeries.h"
#include "FailoverTimeSeries.h"
#include "LagTimeSeries.h"
//#include "WarpingTimeSeries.h"
//#include "ForecastTimeSeries.h"

#include "OutlierExclusionTimeSeries.h"

#include "BufferPointRecord.h"

#include "ConcreteDbRecords.h"

using namespace RTX;
using namespace std;

void printPoints(vector<Point> pointVector);

int main(int argc, const char * argv[])
{
  
  Clock::_sp clock(new Clock(900));
  
  SineTimeSeries::_sp sineWave(new SineTimeSeries());
  sineWave->setName("sine");
  sineWave->setClock(clock);
  
  LagTimeSeries::_sp lag(new LagTimeSeries());
  lag->setOffset(900);
  lag->setSource(sineWave);
  
  time_t t1 = 1400004000; //time(NULL);
  time_t t2 = t1 + 3600;
  vector<Point> points = lag->points(TimeRange(t1, t2));
  
  printPoints(sineWave->points(TimeRange(t1, t2)));
  cout << endl;
  printPoints(points);
  
  /*
  ForecastTimeSeries::_sp pythonTestTs(new ForecastTimeSeries());
  pythonTestTs->setSource(sineWave);
  vector<Point> testPoints = pythonTestTs->points(1000, 3000);
  printPoints(testPoints);
  
  vector<Point> range;
  */
  
  return 0;
}


void printPoints(vector<Point> pointVector) {
  for(Point thePoint : pointVector) {
    cout << thePoint << endl;
  }
}

