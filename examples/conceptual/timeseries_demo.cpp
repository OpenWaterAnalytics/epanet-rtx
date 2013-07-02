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
#include "SineTimeSeries.h"
#include "ConstantTimeSeries.h"
#include "AggregatorTimeSeries.h"
#include "CsvPointRecord.h"
#include "GainTimeSeries.h"

#include "BufferPointRecord.h"
#include "MysqlPointRecord.h"

using namespace RTX;
using namespace std;

void printPoints(vector<Point> pointVector);

int main(int argc, const char * argv[])
{
  
  
  if (0) {
    Clock::sharedPointer reg10m(new Clock(60*10));
    
    ConstantTimeSeries::sharedPointer pressure( new ConstantTimeSeries() );
    pressure->setUnits(RTX_KILOPASCAL);
    pressure->setClock(reg10m);
    pressure->setValue(1);
    
    GainTimeSeries::sharedPointer gainTs( new GainTimeSeries() );
    
    gainTs->setUnits(RTX_FOOT);
    gainTs->setGainUnits( RTX_METER / RTX_PASCAL );
    gainTs->setGain(0.0001019977334);
    gainTs->setSource(pressure);
    
    Point cp = gainTs->point(100);
    
    cout << cp << endl;
    
    double val = Units::convertValue(1, RTX_INCH, RTX_FOOT);
    
    cout << val << endl;
  }
  
  
  
  // RTX TimeSeries Demo Application
  // A demonstration of some of the important TimeSeries classes and methods
  if(0)
  {
    CsvPointRecord::sharedPointer csv(new CsvPointRecord() );
    csv->setReadOnly(false);
    csv->setPath("csv_test");
    
    TimeSeries::sharedPointer ts(new TimeSeries());
    ts->setName("csv1");
    ts->setRecord(csv);
    
    ModularTimeSeries::sharedPointer mts(new ModularTimeSeries());
    mts->setName("mts");
    mts->setSource(ts);
    mts->setRecord(csv);
    
    vector<Point> points = mts->points(10, 30);
    
    
    
  }
  
  cout << "ok" << endl;
  
  
  
  
  
  
  
  
  
  
  
  
  
  /*
   
   The TimeSeries class can store points and give you access to those points.
   We will create some points and store them.
   
   */
  
  using Point::Qual_t::good;
  using Point::Qual_t::missing;
  
  time_t start = 1000000000;
  
  Point  p1(start,   0, good, 0.5),
         p2(start+100, 100, good, 0.8),
         p3(start+120, 120, Point::Qual_t::estimated, 0.8),
         p4(start+230, 230, good, 0.7);

  
  // put these points into a vector
  vector<Point> uglyPoints;
  uglyPoints.push_back(p1);
  uglyPoints.push_back(p2);
  uglyPoints.push_back(p3);
  uglyPoints.push_back(p4);
  
  time_t t = start + 240;
  for (int i=0; i < 10000; ++i) {
    
    uglyPoints.push_back(Point(t,(double)(rand()%100)));
    
    t += (30);
  }
  time_t end = t;
  
  
  PointRecord::sharedPointer pr( new BufferPointRecord() );
  
  // by default, new TimeSeries objects are created with irregular clocks.
  // this is fine, since we're feeding this guy points manually.
  TimeSeries::sharedPointer uglyTimeSeries(new TimeSeries());
  uglyTimeSeries->setName("ugly");
  uglyTimeSeries->setUnits(RTX_MILLION_GALLON_PER_DAY);
  uglyTimeSeries->setRecord(pr);
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
  
  
  ConstantTimeSeries::sharedPointer constantTs( new ConstantTimeSeries() );
  constantTs->setName("constant ts");
  constantTs->setUnits(RTX_MILLION_GALLON_PER_DAY);
  constantTs->setValue(10.);
  constantTs->setClock(regular_30s);
  
  cout << "constant time series:" << endl;
  printPoints(constantTs->points(start, start+240));
  cout << "=======================================" << endl;
  
  
  AggregatorTimeSeries::sharedPointer agg(new AggregatorTimeSeries());
  agg->setName("aggregator");
  agg->setUnits(RTX_MILLION_GALLON_PER_DAY);
  agg->setClock(regular_30s);
  agg->addSource(resampled);
  agg->addSource(constantTs);
  
  cout << "aggregator time series:" << endl;
  vector<Point> ps = agg->points(start+20, start+260);
  printPoints(ps);
  
  
  
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
  dbRecord->dbConnect();
  if (dbRecord->isConnected()) {
    // empty out any previous data
    dbRecord->reset();
    
    // Now I can just hook this record into any of my modular TimeSeries objects...
    movingAverage->setRecord(dbRecord);
    
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
    dbSourceSeries->setRecord(dbRecord); // hook it into the MysqlPointRecord that was created earlier.
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

