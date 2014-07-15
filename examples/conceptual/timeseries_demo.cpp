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

#include "OutlierExclusionTimeSeries.h"

#include "BufferPointRecord.h"
#include "MysqlPointRecord.h"

//#include "SqliteProjectFile.h"

#include "SqlitePointRecord.h"

using namespace RTX;
using namespace std;

void printPoints(vector<Point> pointVector);

int main(int argc, const char * argv[])
{
  
  BufferPointRecord::sharedPointer buff(new BufferPointRecord());
  
  TimeSeries::sharedPointer dirtyTs(new TimeSeries);
  dirtyTs->setName("dirty");
  dirtyTs->setRecord(buff);
  
  
  vector<Point> range;
  
  boost::random::mt19937 rng;         // produces randomness out of thin air
  boost::random::uniform_real_distribution<> oneToOneHundred(1,100);
  
  for (time_t t = 100; t < 10000; t+= 60) {
    range.push_back(Point(t, oneToOneHundred(rng)));
  }
  
  dirtyTs->insertPoints(range);
  
  Clock::sharedPointer windowClock( new Clock(600) );
  windowClock->setPeriod(1200);
  
  OutlierExclusionTimeSeries::sharedPointer outlierTs( new OutlierExclusionTimeSeries() );
  outlierTs->setSource(dirtyTs);
  outlierTs->setWindow(windowClock);
  outlierTs->setExclusionMode(OutlierExclusionTimeSeries::OutlierExclusionModeStdDeviation);
  outlierTs->setOutlierMultiplier(1.);
  
  vector<Point> excluded = outlierTs->points(1000, 3000);
  
  printPoints(range);
  cout << ":::::::::::::::::::::::::::::::::" << endl;
  
  printPoints(excluded);
  
  
  /*
  Units deg_f = RTX_DEGREE_FARENHEIT;
  Units deg_c = RTX_DEGREE_CELSIUS;
  
  cout << " 32 degrees f == " << Units::convertValue(32, RTX_DEGREE_FARENHEIT, RTX_DEGREE_CELSIUS) << " degrees C" << endl;
  cout << " 212 degrees f == " << Units::convertValue(212, RTX_DEGREE_FARENHEIT, RTX_DEGREE_CELSIUS) << " degrees C" << endl;
  cout << " 0 degrees c == " << Units::convertValue(0, RTX_DEGREE_CELSIUS, RTX_DEGREE_FARENHEIT) << " degrees f" << endl;
  cout << " 0 degrees c == " << Units::convertValue(0, RTX_DEGREE_CELSIUS, RTX_DEGREE_KELVIN) << " degrees K" << endl;
  cout << " 0 degrees R == " << Units::convertValue(0, RTX_DEGREE_FARENHEIT, RTX_DEGREE_RANKINE) << " degrees R" << endl;
  */
  
  
  
  Clock::sharedPointer reg10s(new Clock(10));
  
  TimeSeries::sharedPointer sine( new SineTimeSeries(20, 3600) );
  sine->setUnits(RTX_SECOND);
  sine->setClock(reg10s);
  
  TimeSeries::sharedPointer signal( new SineTimeSeries );
  signal->setClock(reg10s);
  signal->setUnits(RTX_MILLION_GALLON_PER_DAY);
  
  WarpingTimeSeries::sharedPointer warping(new WarpingTimeSeries);
  warping->setWarp(sine);
  warping->setSource(signal);
  
  
  printPoints(warping->points(1000, 2000));
  
  
  
  
  
  
//  SineTimeSeries::sharedPointer sine( new SineTimeSeries() );
  
  TimeOffsetTimeSeries::sharedPointer offset( new TimeOffsetTimeSeries() );
  offset->setName("offsetTime");
  offset->setOffset(3600);
  offset->setSource(sine);
  
  
  time_t now = time(NULL);
  time_t oneHour = 60*60;
  
  cout << "sine:" << endl;
  printPoints(sine->points(now, now + 5*oneHour));
  
  cout << "offset:" << endl;
  printPoints(offset->points(now, now + 5*oneHour));
  
  
  
  
  
  
  
  
  
  
  
  
  ConstantTimeSeries::sharedPointer backupTs(new ConstantTimeSeries);
  backupTs->setName("backup");
  backupTs->setValue(12.);
  backupTs->setClock(reg10s);
  
  
  
  
  FailoverTimeSeries::sharedPointer failover(new FailoverTimeSeries);
  failover->setName("failover");
  failover->setSource(dirtyTs);
  failover->setFailoverTimeseries(backupTs);
  
  failover->setMaximumStaleness(25); // 25-second staleness
  
  printPoints(failover->points(104, 210));
  
  
  
  return 0;
}


void printPoints(vector<Point> pointVector) {
  BOOST_FOREACH(Point thePoint, pointVector) {
    cout << thePoint << endl;
  }
}

