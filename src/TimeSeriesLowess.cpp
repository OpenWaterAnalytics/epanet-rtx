#include "TimeSeriesLowess.h"
#include "Lowess.h"

using namespace RTX;
using namespace std;
using PC = RTX::PointCollection;

TimeSeriesLowess::TimeSeriesLowess() {
  _fraction = 0.5;
  this->setSummaryOnly(false);
}

void TimeSeriesLowess::setFraction(double f) {
  
  _fraction = f > 1 ? 1 : ( f < 0 ? 0 : f );
  
  _fraction = f;
  this->invalidate();
}
double TimeSeriesLowess::fraction() {
  return _fraction;
}

PointCollection TimeSeriesLowess::filterPointsInRange(RTX::TimeRange range) {
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->timeBefore(range.start + 1);
    qRange.end = this->source()->timeAfter(range.end - 1);
    qRange.correctWithRange(range);
  }
  
  set<time_t> times = this->timeValuesInRange(qRange);
  auto subranges = this->subRanges(times);
  vector<Point> outPoints;
  outPoints.reserve(subranges.ranges.size());
  
  for (auto &rm : subranges.ranges) {
    // lowess requires in-sample smoothing. in case of gaps, drop the point.
    time_t t = rm.first;
    auto pvr = rm.second;
    TimeRange r = PC::timeRange(pvr);
    
    if (!r.contains(t) || PC::count(pvr) == 0) {
      continue;
    }
    double v = this->valueFromSampleAtTime(pvr,t);
    Point p(t,v);
    if (p.isValid) {
      outPoints.push_back(p);
    }
  }
  
  PointCollection ret(outPoints, this->units());
  if (this->willResample()) {
    ret.resample(times);
  }
  return ret;
}


double TimeSeriesLowess::valueFromSampleAtTime(PointCollection::pvRange r, time_t t) {
  CppLowess::TemplatedLowess<vector<double>, double> lowess;
  
  vector<double> x;
  vector<double> y;
  auto i = r.first;
  while (i != r.second) {
    x.push_back(static_cast<double>(i->time));
    y.push_back(i->value);
    ++i;
  }
  
  vector<double> out(x.size()), tmp1(x.size()), tmp2(x.size());
  
  lowess.lowess(x, y, this->fraction(), 2, 0.0, out, tmp1, tmp2);
  
  vector<Point> outPoints;
  for (size_t i=0; i < x.size(); ++i) {
    outPoints.push_back(Point(static_cast<time_t>(x.at(i)),out.at(i)));
  }
  
  
  
  PointCollection outC(outPoints,this->units());
  
  outC.resample({t});
  
  Point p;
  if (outC.count() > 0) { 
    p = outC.points().front();
  }
  
  return p.value;
}




