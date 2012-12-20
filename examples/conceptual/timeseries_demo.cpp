//
//  timeseries_demo.cpp
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include <iostream>
#include <boost/foreach.hpp>

#include "TimeSeries.h"
#include "Resampler.h"
#include "MovingAverage.h"
#include "FirstDerivative.h"

#include "MysqlPointRecord.h"

using namespace RTX;
using namespace std;

void printPoints(vector<Point> pointVector);

int main(int argc, const char * argv[])
{
  
  // RTX TimeSeries Demo Application
  // A demonstration of some of the important TimeSeries classes and methods
  
  
  /*
   
   The TimeSeries class can store points and give you access to those points.
   We will create some points and store them.
   
   */
  time_t start = 1222873200; // unix-time 2008-10-01 15:00:00 GMT
  
  Point  p1(start,     34.5),
         p2(start+100, 45.2),
         p3(start+120, 45.9),
         p4(start+230, 42.1);

  
  // put these points into a vector
  vector<Point> uglyPoints;
  uglyPoints.push_back(p1);
  uglyPoints.push_back(p2);
  uglyPoints.push_back(p3);
  uglyPoints.push_back(p4);
  
  // by default, new TimeSeries objects are created with irregular clocks.
  // this is fine, since we're feeding this guy points manually.
  TimeSeries::sharedPointer uglyTimeSeries(new TimeSeries());
  uglyTimeSeries->setName("ugly");
  uglyTimeSeries->setUnits(RTX_MILLION_GALLON_PER_DAY);
  uglyTimeSeries->insertPoints(uglyPoints);
  
  // print out the information we have so far:
  cout << *uglyTimeSeries << endl;
  
  // to retrieve a particular point, we just ask for it.
  // notice that we are asking for a point in time that the uglyTimeSeries doesn't know about.
  // the best it can do is find the point just prior to our query and return that instead.
  Point myPoint = uglyTimeSeries->point(start+110);
  cout << "single point: " << myPoint << endl << endl;
  
  // if we want to retrieve the points in this time series for some range, we can do so:
  cout << "range of points:" << endl;
  printPoints( uglyTimeSeries->points(start-10, start+240) );
  
  cout << "=======================================" << endl;
  
  
  
  
  ModularTimeSeries::sharedPointer modularSeries( new ModularTimeSeries() );
  modularSeries->setName("modular series");
  modularSeries->setSource(uglyTimeSeries);
  cout << "modular time series:" << endl;
  printPoints( modularSeries->points(start, start+240) );
  
  
  
  
  
  // This time series is, as its name would suggest, ugly.
  // It's irregular and noisy. How to clean up?
  // First, we can resample to regular intervals. Perhaps 10-seconds.
  Clock::sharedPointer regular_30s(new Clock(30));
  Resampler::sharedPointer resampled(new Resampler());
  resampled->setName("resampled but still ugly");
  resampled->setClock(regular_30s);
  // resampled TS automagically takes on the units of its source,
  // so we don't have to set that manually, as above, unless
  // we want to change units.
  resampled->setSource(uglyTimeSeries);
  
  // print info about our resampler
  cout << endl << *resampled;
  // display a range of points, as before:
  cout << "range of resampled points" << endl;
  printPoints( resampled->points(start, start+240) );
  
  cout << "=======================================" << endl;
  
  // how about smoothing?
  MovingAverage::sharedPointer movingAverage(new MovingAverage());
  movingAverage->setName("pretty moving average");
  movingAverage->setWindowSize(5);
  movingAverage->setSource(resampled);
  // changing the units 
  movingAverage->setUnits(RTX_MILLION_GALLON_PER_DAY);
  // print info on this new timeseries:
  cout << endl << *movingAverage;
  cout << "range of smoothed points:" << endl;
  printPoints( movingAverage->points(start, start+240) );
  
  cout << "=======================================" << endl;
  
  // how about a change of units? conversion is handled automatically.
  cout << "in GPM:" << endl;
  movingAverage->setUnits(RTX_GALLON_PER_MINUTE);
  printPoints( movingAverage->points(start, start+240) );
  cout << "=======================================" << endl;
  
  // Great. Now let's say I want to persist some of this TimeSeries data. Then what?
  // I can use the MysqlPointRecord (fill in your credentials below):
  MysqlPointRecord::sharedPointer dbRecord(new MysqlPointRecord());
  dbRecord->setConnectionString("HOST=tcp://localhost;UID=rtx_db_agent;PWD=rtx_db_agent;DB=rtx_demo_db");
  dbRecord->connect();
  if (dbRecord->isConnected()) {
    // empty out any previous data
    dbRecord->reset();
    
    // Now I can just hook this record into any of my modular TimeSeries objects...
    movingAverage->newCacheWithPointRecord(dbRecord);
    
    // And when I call the point() or points() methods, the data will automatically be persisted
    // to the MySQL database instead of just RAM.
    // Really - Check your database after this call.
    cout << endl << "points are being persisted:" << endl;
    printPoints( movingAverage->points(start, start+240) );
  }
  else {
    cout << "whoops, db could not connect" << endl;
  }
  
  cout << "=======================================" << endl;
  
  // Does it work the other way around?
  // We just created points, pushed them into a TimeSeries, performed some operations on the TimeSeries
  // (resample, then moving average), and stored all that into a database.
  // Can we now hook a new TimeSeries (which knows nothing of the previous steps) into that database source? Sure thing.
  if (dbRecord->isConnected()) {
    TimeSeries::sharedPointer dbSourceSeries(new TimeSeries());
    dbSourceSeries->setUnits(RTX_GALLON_PER_MINUTE);
    dbSourceSeries->setName("pretty moving average"); // the name associated with the time series that we persisted above.
    dbSourceSeries->newCacheWithPointRecord(dbRecord); // hook it into the MysqlPointRecord that was created earlier.
    cout << endl << "points retrieved from the db:" << endl;
    printPoints( dbSourceSeries->points(start, start+240) );
  }
  
  
  cout << "=======================================" << endl;
  
  
  FirstDerivative::sharedPointer derivative( new FirstDerivative() );
  derivative->setSource(movingAverage);
  
  printPoints(derivative->points(start, start+240));
  
  
  return 0;
}


void printPoints(vector<Point> pointVector) {
  BOOST_FOREACH(Point thePoint, pointVector) {
    cout << thePoint << endl;
  }
}

