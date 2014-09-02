//
//  ForecastTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 8/26/14.
//
//

#include "ForecastTimeSeries.h"


using namespace RTX;
using namespace std;


ForecastTimeSeries::ForecastTimeSeries() {
  _ar = 0;
  _ma = 0;
  
  _fitTime = 0;
}


vector<Point> ForecastTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  
  
}
