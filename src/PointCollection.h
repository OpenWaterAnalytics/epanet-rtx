#ifndef PointCollection_h
#define PointCollection_h

#include <stdio.h>
#include <vector>
#include <set>
#include <functional>

#include "Units.h"
#include "TimeRange.h"
#include "Point.h"

namespace RTX {
  
  typedef enum {
    ResampleModeLinear = 0,
    ResampleModeStep = 1
  } ResampleMode;
  
  /// PointCollection is a wrapper for a point vector, with some intelligence for sampling and subranging
  
  class PointCollection {
  public:
    typedef std::vector<Point>::iterator pvIt;
    typedef std::pair<pvIt,pvIt> pvRange;
    
    PointCollection(std::vector<Point> points, Units units);
    PointCollection();
    
    void apply(std::function<void(Point&)> function) const;
    pvRange raw() const;
    std::vector<Point> points() const;
    void setPoints(std::vector<Point> points);
    
    Units units;
    const std::set<time_t> times() const;
    TimeRange range() const;
    
    bool resample(std::set<time_t> timeList, ResampleMode mode = ResampleModeLinear);
    bool convertToUnits(Units u);
    void addQualityFlag(Point::PointQuality q);
    
    // statistical methods on point collections
    
    static double min(pvRange r);
    static double max(pvRange r);
    static double mean(pvRange r);
    static double variance(pvRange r);
    static size_t count(pvRange r);
    static double percentile(double p, pvRange r);
    static double interquartilerange(pvRange r);
    static TimeRange timeRange(pvRange r);
    
    double min() const { return PointCollection::min(this->raw()); };
    double max() const { return PointCollection::min(this->raw()); };
    double mean() const { return PointCollection::mean(this->raw()); };
    double variance() const { return PointCollection::variance(this->raw()); };
    size_t count() const { return _points->size(); };
    double percentile(double p) const { return PointCollection::percentile(p,this->raw()); };
    double interquartilerange() const { return PointCollection::interquartilerange(this->raw()); };
    TimeRange timeRange() const { return PointCollection::timeRange(this->raw()); };
    
    
    // non-mutating
    pvRange subRange(TimeRange r, pvRange range_hint = pvRange()) const;
    PointCollection trimmedToRange(TimeRange range) const;
    PointCollection resampledAtTimes(const std::set<time_t>& times, ResampleMode mode = ResampleModeLinear) const;
    PointCollection asDelta() const;
    
  private:
    std::shared_ptr< std::vector<Point> > _points;
  };
}


#endif /* PointCollection_h */
