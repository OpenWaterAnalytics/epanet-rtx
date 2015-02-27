//
//  InpTextPattern.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/27/15.
//
//

#include <sstream>
#include <boost/foreach.hpp>
#include "InpTextPattern.h"

#include "TimeSeriesFilter.h"

using namespace RTX;
using namespace std;

std::string InpTextPattern::textPatternWithTimeSeries(TimeSeries::_sp ts, const std::string& patternName, time_t from, time_t to, int step, TimeSeries::TimeSeriesResampleMode interp) {

  stringstream patStream;
  
  Clock::_sp patClock( new Clock(step,from));
  
  TimeSeriesFilter::_sp filter(new TimeSeriesFilter());
  filter->setName(patternName);
  filter->setSource(ts);
  filter->setClock(patClock);
  filter->setResampleMode(interp);
  
  TimeSeries::PointCollection patternData = filter->pointCollection(TimeRange(from,to));
  
  patStream << "; " << patternName << endl;
  
  
  uint i = 0;
  uint lineLength = 12;
  
  BOOST_FOREACH(const Point& p, patternData.points) {
    // start of line?
    if (i == 0) {
      patStream << patternName << "    ";
    }
    ++i;
    
    patStream << p.value << "    ";
    
    // line break?
    if (i > lineLength) {
      i = 0;
      patStream << endl;
    }
  }
  
  
  const string patternStr(patStream.str());
  return patternStr;
}