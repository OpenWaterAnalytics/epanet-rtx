#include "TimeSeriesLowess.h"
#include "Lowess.h"

using namespace RTX;
using namespace std;


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

TimeSeries::PointCollection TimeSeriesLowess::filterPointsInRange(RTX::TimeRange range) {
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->timeBefore(range.start + 1);
    qRange.end = this->source()->timeAfter(range.end - 1);
    qRange.correctWithRange(range);
  }
  
  set<time_t> times = this->timeValuesInRange(qRange);
  auto smr = this->filterSummaryCollection(times);
  vector<Point> outPoints;
  outPoints.reserve(smr->summaryMap.size());
  
  for (auto &sumPair : smr->summaryMap) {
    
    time_t t = sumPair.first;
    PointCollection c = sumPair.second;
    if (c.count() == 0) {
      continue;
    }
    double v = this->valueFromCollectionAtTime(c,t);
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


double TimeSeriesLowess::valueFromCollectionAtTime(TimeSeries::PointCollection c, time_t t) {
  
  
  CppLowess::TemplatedLowess<vector<double>, double> lowess;
  
  vector<double> x;
  vector<double> y;
  for (auto p : c.points()) {
    x.push_back(static_cast<double>(p.time));
    y.push_back(p.value);
  }
  
  vector<double> out(x.size()), tmp1(x.size()), tmp2(x.size());
  
  lowess.lowess(x, y, this->fraction(), 2, 0.0, out, tmp1, tmp2);
  
  vector<Point> outPoints;
  for (int i=0; i < x.size(); ++i) {
    outPoints.push_back(Point(static_cast<time_t>(x.at(i)),out.at(i)));
  }
  
  
  
  PointCollection outC(outPoints,this->units());
  
  outC.resample({t});
  
  Point p = outC.points().front();
  
  return p.value;
}




