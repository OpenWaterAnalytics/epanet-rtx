//
//  MovingAverage.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include "MovingAverage.h"
#include "PersistentContainer.h"
#include <boost/foreach.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <math.h>

#include <iostream>

using namespace RTX;
using std::cout;
using namespace boost::accumulators;


MovingAverage::MovingAverage() : ModularTimeSeries::ModularTimeSeries() {
  _windowSize = 7;
}

MovingAverage::~MovingAverage() {
    
}    
/* i don't know what this is supposed to do ... -sam 
// create a "source" time series by specifying a source name and a point record name to connect to the source.
vector<Point::sharedPointer> MovingAverage::sourceTimeSeries(const std::string& nameSource, PointRecord::sharedPointer pointRecord, time_t tStart, long tEnd) {
    TimeSeries::sharedPointer myTimeSeries( new TimeSeries(nameSource) );
    myTimeSeries->newCacheWithPointRecord(pointRecord);
    vector< Point::sharedPointer > somePoints = myTimeSeries->points(tStart, tStart + tEnd);
    return somePoints;
}
*/

#pragma mark - Added Methods

void MovingAverage::setWindowSize(int numberOfPoints) {
  resetCache();
  _windowSize = numberOfPoints;
}

int MovingAverage::windowSize() {
  return _windowSize;
}

#pragma mark - Public Overridden Methods

Point::sharedPointer MovingAverage::point(time_t time) {
  
  // check the requested time for validity...
  if ( !(clock()->isValid(time)) ) {
    // if the time is not valid, rewind until a valid time is reached.
    time = clock()->timeBefore(time);
  }
  
  // a point is requested. see if it is available in my cache (via base class methods)
  if (TimeSeries::isPointAvailable(time)) {
    return TimeSeries::point(time);
  }
  
  // if not, we need to construct it and store it locally.
  Point::sharedPointer aFilteredPoint = Point::convertPoint(this->movingAverageAt(time), source()->units(), units());
  
  // add the point to the local cache, and return it.
  this->insert(aFilteredPoint);
  
  return aFilteredPoint;
}

std::vector< Point::sharedPointer > MovingAverage::points(time_t start, time_t end) {
  // todo - better logic here...
  time_t period = this->period();
  
  // encourage the appropriate cache.
  if (!source()) {
    std::vector< Point::sharedPointer > empty;
    return empty;
  }
  source()->points( start - (period * windowSize()), end + (period * windowSize()) );
  
  // now return just the results we care about.
  return ModularTimeSeries::points(start, end);
  
}



#pragma mark - Private Methods

Point MovingAverage::movingAverageAt(time_t time) {
  
  // get a collection of points around "time" from the source() timeseries, and send them to the calculator.
  
  std::vector< Point::sharedPointer > somePoints;
  int halfWindow = floor(_windowSize / 2);
  
  // push in the point at the current time
  somePoints.push_back( source()->point(time) );
  
  // push in a half-window's worth of points (to the left) from the source time series into the vector.
  time_t rewindTime = time;
  Point::sharedPointer pointToPush;
  for (int i = 0; i < halfWindow; ++i) {
    pointToPush = source()->pointBefore(rewindTime);
    if (!pointToPush) {
      continue;
    }
    somePoints.push_back( pointToPush );
    rewindTime = pointToPush->time();
  }
  
  // and now push a half-window's worth of points from the right.
  time_t forwardTime = time;
  for (int i = 0; i < halfWindow; ++i) {
    pointToPush = source()->pointAfter(forwardTime);
    if (!pointToPush) {
      continue;
    }
    somePoints.push_back( pointToPush );
    forwardTime = pointToPush->time();
  }
  
  
  double movingAverageValue = calculateAverage(somePoints);
    
  return Point(time, movingAverageValue, Point::good);
  
}

// average the points
double MovingAverage::calculateAverage(vector< Point::sharedPointer > somePoints) {
  
  accumulator_set<double, stats<tag::mean> > meanAccumulator;
  BOOST_FOREACH(Point::sharedPointer point, somePoints) {
    double pointValue = point->value();
    meanAccumulator(pointValue);
  }
  
  double meanValue = mean(meanAccumulator);
  return meanValue;

}