//
//  TimeSeriesFilterSinglePoint.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/16/15.
//
//

#include "TimeSeriesFilterSinglePoint.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;



TimeSeries::PointCollection TimeSeriesFilterSinglePoint::filterPointsAtTimes(std::set<time_t> times) {
  PointCollection sourceRaw = this->source()->resampled(times);
  vector<Point> outPoints;
  
  BOOST_FOREACH(const Point& sourcePoint, sourceRaw.points) {
    Point converted = this->filteredWithSourcePoint(sourcePoint);
    outPoints.push_back(converted);
  }
  
  return PointCollection(outPoints, this->units());
}

