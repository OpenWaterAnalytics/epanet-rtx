//
//  FirstDerivative.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "FirstDerivative.h"

using namespace std;
using namespace RTX;

FirstDerivative::FirstDerivative() {
  
}

FirstDerivative::~FirstDerivative() {
  
}

void FirstDerivative::setSource(TimeSeries::sharedPointer source) {
  Units originalUnits = this->units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  Resampler::setSource(source);
  if (source) {
    // get the rate of change units
    Units rate = source->units() / RTX_SECOND;
    
    if (rate.isSameDimensionAs(originalUnits)) {
      rate = originalUnits;
    }
    this->setUnits(rate);  // re-set the units.
  }
  
}

void FirstDerivative::setUnits(Units newUnits) {
  
  // only set the units if there is no source or the source's rate is dimensionally consistent with the passed-in units.
  if (!source() || (source()->units() / RTX_SECOND).isSameDimensionAs(newUnits) ) {
    // just use the base-est class method for this, since we don't really care
    // if the new units are the same as the source units.
    TimeSeries::setUnits(newUnits);
  }
  else if (!units().isDimensionless()) {
    std::cerr << "units are not dimensionally consistent" << std::endl;
  }
}

Point FirstDerivative::filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits) {
  
  Point fd;
  
  // quick sanity check
  if (vecPos == vecEnd) {
    return Point();
  }
  
  pVec_cIt fwd_it = vecPos;
  pVec_cIt back_it = vecPos;
  bool ok = alignVectorIterators(vecStart, vecEnd, vecPos, t, back_it, fwd_it);
  
  
  // with any luck, at this point we have the back and fwd iterators positioned just right.
  // one on either side of the time point we need.
  // however, we may have been unable to accomplish this task.
  //if (t < back_it->time || fwd_it->time < t) {
  if (!ok) {
    return Point(); // invalid
  }
  
  // it's possible that the vecPos is aligned right on the requested time, so check for that:
  if (vecPos->time == t) {
    // do back-calculation. forget the fwd iterator, use the vecPos instead.
    fwd_it = vecPos;
  }
  Point p1, p2;
  p1 = *back_it;
  p2 = *fwd_it;
  
  time_t dt = p2.time - p1.time;
  double dv = p2.value - p1.value;
  double dvdt = Units::convertValue(dv / double(dt), fromUnits / RTX_SECOND, this->units());
  
  return Point(t, dvdt);
}



std::ostream& FirstDerivative::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  if (source()) {
    stream << "First Derivative Of: " << *source() << "\n";
  }
  
  return stream;
}
