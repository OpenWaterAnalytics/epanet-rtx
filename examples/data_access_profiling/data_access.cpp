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
    
    
    MysqlPointRecord::sharedPointer record( new MysqlPointRecord() );
    record->setHost("unix:///tmp/mysql.sock");
    record->setUid("rtx_db_agent");
    record->setPwd("rtx_db_agent");
    record->setDb("rtx_demo_db");
    record->dbConnect();
    cout << *record;
    sourceTS->setRecord(record);
    
    // clear points
    record->reset(sourceTS->name());
    
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
  MysqlPointRecord::sharedPointer record2( new MysqlPointRecord() );
  record2->setHost("tcp://localhost");
  record2->setUid("rtx_db_agent");
  record2->setPwd("rtx_db_agent");
  record2->setDb("rtx_demo_db");
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
