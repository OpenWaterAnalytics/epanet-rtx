//
//  main.cpp
//  data_access_profiling
//

#include <ctime>
#include <iostream>
#include <iomanip>
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

#include "InfluxDbPointRecord.h"

vector<Point> randomPoints(time_t start, int nPoints, time_t period = 0);

int main(int argc, const char * argv[])
{
  
  InfluxDbPointRecord::_sp influxDb( new InfluxDbPointRecord() );
  
  influxDb->setName("Influx");
  influxDb->setConnectionString("host=localhost&port=8086&db=testing&u=root&p=root");
  
  influxDb->dbConnect();
  
  
  Clock::_sp reg_5m( new Clock(5*60) );
  
  
  
  ConstantTimeSeries::_sp constantTs( new ConstantTimeSeries );
  Clock::_sp oneMinute (new Clock(60));
  constantTs->setClock(oneMinute);
  constantTs->setValue(100.221);
  
  SineTimeSeries::_sp sine( new SineTimeSeries );
  sine->setClock(oneMinute);
  
  TimeSeriesFilter::_sp mod(new TimeSeriesFilter);
  mod->setName("sys.gcww.dma.lebanon.demand");
  mod->setSource(sine);
  mod->setRecord(influxDb);
  
  influxDb->invalidate("ts_test");
  
  
  time_t start = 1420088400; // the year two thousand fifteen!!!
  time_t end = start + 60*60*24*365;
  time_t increment = 60*60*24;
  time_t chunk;
  
  
  
  
  
  {
    boost::timer::auto_cpu_timer t;
    chunk = start;
    while (chunk <= end) {
      time_t thisEnd = (end < chunk + increment) ? end : chunk + increment;
      mod->points(chunk, thisEnd);
      chunk += increment;
    }
    
    cout << "influx results " << endl;
  }
  
  
  
  SqlitePointRecord::_sp sqliteRecord(new SqlitePointRecord);
  sqliteRecord->setPath("/tmp/sqlite_test.sqlite");
  sqliteRecord->dbConnect();
  mod->setRecord(sqliteRecord);
  
  {
    boost::timer::auto_cpu_timer t;
    chunk = start;
    while (chunk <= end) {
      mod->points(chunk, chunk + increment);
      chunk += increment;
    }
    
    cout << "sqlite results " << endl;
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
