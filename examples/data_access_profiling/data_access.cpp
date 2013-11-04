//
//  main.cpp
//  data_access_profiling
//


#include <iostream>
#include <iomanip>
#include <boost/timer/timer.hpp>
#include <boost/foreach.hpp>

#include "TimeSeries.h"
#include "Resampler.h"
#include "MovingAverage.h"
#include "PointRecord.h"
#include "MysqlPointRecord.h"

using namespace std;
using namespace RTX;

vector<Point> randomPoints(time_t start, int nPoints, time_t period = 0);

int main(int argc, const char * argv[])
{
  
  
  
  cout << setfill('-') << setw(30) << "-" << endl;
  
  
  time_t start = 10;
  time_t period = 60;
  int nPoints = 100000;
  
  
  std::vector<Point> sourcePoints = randomPoints(start, nPoints, period);
  time_t end = sourcePoints.back().time;
  
  
  
  {
    TimeSeries::sharedPointer sourceTS( new TimeSeries() );
    sourceTS->setName("source");
    sourceTS->setUnits(RTX_CUBIC_FOOT_PER_SECOND);
    
    DbPointRecord::sharedPointer record( new MysqlPointRecord() );
    record->setConnectionString("HOST=unix:///tmp/mysql.sock;UID=rtx_db_agent;PWD=rtx_db_agent;DB=rtx_benchmark");
    record->dbConnect();
    cout << *record;
    
    // clear points
    record->reset();
    sourceTS->setRecord(record);
    
    Clock::sharedPointer reg( new Clock(1) );
    
    MovingAverage::sharedPointer ma( new MovingAverage );
    ma->setClock(reg);
    ma->setSource(sourceTS);
    ma->setWindowSize(7);
    ma->setName("movingaverage");
    ma->setRecord(record);
    
    time_t startTime = 1382721461;
    time_t increment = 1;
    while (1) {
      time_t summaryStart = startTime;
      std::vector<Point> rando;
      for (time_t t=0; t < 100000; t+=increment) {
        startTime += increment;
        double pointValue = (double)t;
        Point p(startTime, pointValue);
        rando.push_back(p);
      }
      time_t summaryEnd = startTime;
      {
        boost::timer::auto_cpu_timer insTimer;
        sourceTS->insertPoints(rando);
        TimeSeries::Summary summary = ma->summary(summaryStart + 4, summaryEnd - 4);
      }
      
      
    }
    
    
    
    
    
    
    
    
    
    
    if (0) {
      cout << "single insertion:" << endl;
      boost::timer::auto_cpu_timer insTimer;
      for (vector<Point>::const_iterator pIt = sourcePoints.begin(); pIt != sourcePoints.end(); ++pIt) {
        sourceTS->insert(*pIt);
      }
    } // insTimer
    
    // clear the points
    record->reset(sourceTS->name());
    
    {
      cout << "range insertion:" << endl;
      boost::timer::auto_cpu_timer bulkInsTimer;
      sourceTS->insertPoints(sourcePoints);
    }// bulkInsTimer
  }
  
  // for selection profiling, make sure no points are cached.
  // this will create a new time series that uses the same point record as the original sourceTS.
  DbPointRecord::sharedPointer record2( new MysqlPointRecord() );
  record2->setConnectionString("HOST=tcp://localhost;UID=rtx_db_agent;PWD=rtx_db_agent;DB=rtx_demo_db");
  record2->dbConnect();
  TimeSeries::sharedPointer ts(new TimeSeries());
  ts->setName("source");
  ts->setUnits(RTX_CUBIC_FOOT_PER_SECOND);
  ts->setRecord(record2);
  
  
  {
    cout << "resampler range access:" << endl;
    ModularTimeSeries::sharedPointer mts( new Resampler() );
    mts->setName("resampler");
    mts->setSource(ts);
    Clock::sharedPointer reg_30(new Clock(30));
    mts->setClock(reg_30);
    //mts->setRecord(record);
    boost::timer::auto_cpu_timer resTimer;
    vector<Point> pvec = mts->points(start, end);
    cout << "found " << pvec.size() << " points" << endl;
  }// resTimer
  
}






vector<Point> randomPoints(time_t start, int nPoints, time_t period) {
  vector<time_t> times;
  times.reserve(nPoints);
  
  if (period == 0) {
    // random times.
    for (int i = 0; i < nPoints; ++i) {
      time_t randTime = rand() % nPoints*60*10 + 1;
      times.push_back(randTime+start);
    }
    std::sort(times.begin(), times.end());
    
  }
  else {
    for (int i = 0; i < nPoints; i++) {
      times.push_back(start + i*period);
    }
  }
  
  vector<Point> points;
  points.reserve(nPoints);
  
  BOOST_FOREACH(time_t t, times) {
    double value = rand() % 100;
    points.push_back(Point(t, value));
  }
  
  return points;
}
