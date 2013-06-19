//
//  RuntimeStatus.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "RuntimeStatus.h"

using namespace std;
using namespace RTX;

RuntimeStatus::RuntimeStatus() {
  _threshold = 0; // runtime change > 0 _seconds_ means on
  _offValue = 0; // off status value
  _onValue = 1; // on status value
}

RuntimeStatus::~RuntimeStatus() {
  
}

void RuntimeStatus::setThreshold(double threshold) {
  _threshold = threshold;
}

double RuntimeStatus::threshold() {
  return _threshold;
}

void RuntimeStatus::setSource(TimeSeries::sharedPointer source) {
  TimeSeries::setUnits(RTX_SECOND);  // temporarily set time units to require a runtime source
  Resampler::setSource(source);
  TimeSeries::setUnits(RTX_DIMENSIONLESS);  // But our units are dimensionless on/off status
}

void RuntimeStatus::setUnits(Units newUnits) {
  // Status is a dimensionless on/off; can't be set
  if (!newUnits.isDimensionless()) {
    std::cerr << "units are not dimensionally consistent" << std::endl;
  }
}

Point RuntimeStatus::filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits) {
  
  // quick sanity check
  if (vecPos == vecEnd) {
    return Point();
  }
    
  // move the position vector to my desired time, or slightly after.
  while (vecPos != vecEnd && vecPos->time < t) {
    ++vecPos;
  }

  // position back and fwd iterators on either side of position
  pVec_cIt fwd_it = vecPos;
  pVec_cIt back_it = vecPos;
  ++fwd_it;
  --back_it;

  // with any luck, at this point we have the back and fwd iterators positioned just right.
  // one on either side of the time point we need.
  // however, we may have been unable to accomplish this task.
  if (t < back_it->time || fwd_it->time < t) {
    return Point(); // invalid
  }
  
  Point pBack, pNow, pFwd;
  pBack = *back_it;
  pNow = *vecPos;
  pFwd = *fwd_it;
  
  double runTime1 = Units::convertValue(pNow.value - pBack.value, fromUnits, RTX_SECOND);
  double runTime2 = Units::convertValue(pFwd.value - pNow.value, fromUnits, RTX_SECOND);
  time_t newT;
  double newStatus;
  if (runTime1 <= _threshold && runTime2 > _threshold) { // Switch from off to on status
    newT = pFwd.time - (time_t)runTime2; // time of switch
    newStatus = (t >= newT) ? _onValue : _offValue;
  }
  else if (runTime1 > _threshold && runTime2 <= _threshold) { // Switch from on to off status
    newT = pBack.time + (time_t)runTime1; // time of switch
    newStatus = (t >= newT) ? _offValue : _onValue;
  }
  else if (runTime1 > _threshold && runTime2 > _threshold) { // Continuing on status
    newT = pBack.time + runTime1 + runTime2; // 
    newStatus = (t >= newT) ? _offValue : _onValue;
  }
  else { // Continuing off status
    newT = t;
    newStatus = _offValue;
  }
  
  if (this->clock()->isRegular()) {
    // we are resampling, so return the status at the resampling time
    return Point(t, newStatus);
  }
  else {
    // there is no clock, so feel free to return the actual time of a status change.
    return Point(newT,newStatus);
  }
}



std::ostream& RuntimeStatus::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Runtime Status of: " << *source() << "\n";
  return stream;
}

int RuntimeStatus::margin() {
  return 2;
}
