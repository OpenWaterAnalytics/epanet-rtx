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



TimeSeries::PointCollection TimeSeriesFilterSinglePoint::filterPointsInRange(TimeRange range) {
  
  TimeRange queryRange = range;
  if (this->willResample()) {
    // expand range

    Point seekLeft = this->source()->pointBefore(range.start + 1);
    while (seekLeft.time > 0 && !this->filteredWithSourcePoint(seekLeft).isValid) {
      seekLeft = this->source()->pointBefore(seekLeft.time);
    }
    
    queryRange.start = seekLeft.time;
    
    Point seekRight = this->source()->pointAfter(range.end -1 );
    while (seekRight.time > 0 && !this->filteredWithSourcePoint(seekRight).isValid) {
      seekRight = this->source()->pointAfter(seekRight.time);
    }
    
    queryRange.end = seekRight.time;
  }
  
  PointCollection data = source()->points(queryRange);
  
  vector<Point> outPoints;
  bool didDropPoints = false;
  
  BOOST_FOREACH(const Point& sourcePoint, data.points) {
    Point converted = this->filteredWithSourcePoint(sourcePoint);
    if (converted.isValid) {
      outPoints.push_back(converted);
    }
    else {
      didDropPoints = true;
    }
  }
  
  PointCollection outData(outPoints, this->units());
  if (this->willResample() || (didDropPoints && this->clock())) {
    set<time_t> timeValues = this->timeValuesInRange(range);
    outData.resample(timeValues);
  }
  
  return outData;
}

