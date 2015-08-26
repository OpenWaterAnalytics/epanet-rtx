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
#include "ConstantTimeSeries.h"
#include "SineTimeSeries.h"
#include "MovingAverage.h"
#include "PointRecord.h"
#include "MysqlPointRecord.h"
#include "SqlitePointRecord.h"
//#include "OdbcDirectPointRecord.h"
#include "ThresholdTimeSeries.h"


using namespace std;
using namespace RTX;

//#include "InfluxDbPointRecord.h"

vector<Point> randomPoints(time_t start, int nPoints, time_t period = 0);

void profile_no_prefetch(vector<TimeSeries::_sp> tsvec, time_t start, time_t duration);
void profile_prefetch(vector<TimeSeries::_sp> tsvec, time_t start, time_t duration);



int main(int argc, const char * argv[])
{
  
  int n_series = 1000;
  
  time_t duration = (60*60*24*7);
  time_t start = time(NULL) - duration;
  time_t period = 60*10;
  
  vector<TimeSeries::_sp> tsCollection;
  tsCollection.reserve(n_series);
  
  SqlitePointRecord::_sp sqlitepr(new SqlitePointRecord);
  sqlitepr->setPath("/Users/sam/Desktop/small_profile.db");
  sqlitepr->dbConnect();
  
  {
    {
      boost::timer::auto_cpu_timer t;
      cout << "setting up time series objects" << endl;
      for (int i = 0; i < n_series; ++i) {
        TimeSeries::_sp ts(new TimeSeries);
        stringstream ss;
        ss << "Series_" << i;
        ts->setName(ss.str());
        ts->setRecord(sqlitepr);
        tsCollection.push_back(ts);
      }
    }
    {
      boost::timer::auto_cpu_timer t;
      cout << "sending data" << endl;
      
      BOOST_FOREACH(TimeSeries::_sp ts, tsCollection) {
        vector<Point> rando = randomPoints(start, (int)(duration/period), period);
        ts->insertPoints(rando);
      }
    }
  }
  
  
  {
    boost::timer::auto_cpu_timer t;
    cout << "resetting caches" << endl;
    BOOST_FOREACH( TimeSeries::_sp ts, tsCollection) {
      ts->resetCache();
    }
  }
  
  
  
  
  profile_no_prefetch(tsCollection, start, duration);
  profile_prefetch(tsCollection, start, duration);
  
  
  
  
}


void profile_no_prefetch(vector<TimeSeries::_sp> tsvec, time_t start, time_t duration) {
  {
    boost::timer::auto_cpu_timer t;
    cout << "no prefetch: get all points in range from each series" << endl;
    BOOST_FOREACH( TimeSeries::_sp ts, tsvec) {
      Point p = ts->pointAfter(start);
      while (p.time < start + duration - 60*60 && p.isValid) {
        p = ts->pointAfter(p.time);
      }
    }
  }
}

void profile_prefetch(vector<TimeSeries::_sp> tsvec, time_t start, time_t duration) {
  {
    boost::timer::auto_cpu_timer t;
    cout << "fetching " << tsvec.size() << " timeseries from db..." << endl;
    BOOST_FOREACH( TimeSeries::_sp ts, tsvec) {
      ts->points(TimeRange(start, start + duration));
    }
  }
  
  {
    boost::timer::auto_cpu_timer t;
    cout << "with prefetch: get all points in range from each series" << endl;
    BOOST_FOREACH( TimeSeries::_sp ts, tsvec) {
      Point p = ts->pointAfter(start);
      while (p.time < start + duration - 60*60 && p.isValid) {
        p = ts->pointAfter(p.time);
      }
    }
  }
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
