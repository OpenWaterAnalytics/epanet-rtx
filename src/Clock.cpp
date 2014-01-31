//
//  File.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Clock.h"

using namespace RTX;

Clock::Clock(int period, time_t start) : _name("") {
  if (period > 0) {
    _period = period;
    _isRegular = true;
  }
  else {
    _period = 0;
    _isRegular = false;
  }
  _start = start;
}

Clock::~Clock() {
  // empty
}

std::ostream& RTX::operator<< (std::ostream &out, Clock &clock) {
  return clock.toStream(out);
}

#pragma mark - Public Methods


std::string Clock::name() {
  return _name;
}

void Clock::setName(std::string name) {
  _name = name;
}

bool Clock::isCompatibleWith(Clock::sharedPointer clock) {
  // if this clock is irregular, it's compatible with anything!
  if (!isRegular()) {
    return true;
  }
  
  // otherwise...
  time_t offsetDifference;
  int periodModulus;
  offsetDifference = this->start() - clock->start();
  if (offsetDifference < 0) {
    offsetDifference *= -1;
  }
  // determine if there's a difference in period - it's ok if the compared
  // clock is faster, as long as it's a multiple.
  if (clock->period() > 0) {
    periodModulus = this->period() % clock->period();
  }
  else {
    return false;
  }
  
  
  // if the difference is evenly divisible by the compared period,
  // and the periods are evenly divisible... then it's ok.
  if ( (offsetDifference % clock->period() == 0) && (periodModulus == 0) ) {
    return true;
  }
  else return false;
  
}


bool Clock::isValid(time_t time) {
  
  if (!isRegular() || timeOffset(time) == 0) {
    return true;
  }
  else {
    return false;
  }
}

bool Clock::isRegular() {
  return _isRegular;
}

int Clock::period() {
  return _period;
}

void Clock::setPeriod(int p) {
  _period = p;
  if (p > 0) {
    _isRegular = true;
  }
}

time_t Clock::start() {
  return _start;
}

void Clock::setStart(time_t startTime) {
  _start = startTime;
}

time_t Clock::validTime(time_t time) {
  if ( !isValid(time) ) {
    // if the time is not valid, rewind until a valid time is reached.
    time = timeBefore(time);
  }
  return time;
}

time_t Clock::timeBefore(time_t time) {
  if (!isRegular()) {
    return (time - 1);
  }
  if (isValid(time)) {
    // return the previous time value
    return ( time - period() );
  }
  else {
    // find the closest previous time value that falls on the regular interval
    return ( time - timeOffset(time));
  }
}

time_t Clock::timeAfter(time_t time) {
  if (!isRegular()) {
    return (time + 1);
  }
  if (isValid(time)) {
    // return the next time value
    return ( time + period() );
  }
  else {
    // find the next time value that falls on the regular interval
    return ( time - timeOffset(time) + period());
  }
}

std::vector< time_t > Clock::timeValuesInRange(time_t start, time_t end) {
  std::vector<time_t> timeList;
  if (!isValid(start)) {
    start = timeAfter(start);
  }
  for (time_t thisTime = start; thisTime <= end; thisTime = timeAfter(thisTime)) {
    if (thisTime == 0) {
      break;
    }
    timeList.push_back(thisTime);
  }
  return timeList;
}


#pragma mark - Protected Methods

std::ostream& Clock::toStream(std::ostream &stream) {
  if (isRegular()) {
    stream << "period(" << period() << ") ";
    stream << "offset(" << start() << ")\n";
  }
  else {
    stream << "irregular\n";
  }
  
  return stream;
}


#pragma mark - Private Methods

time_t Clock::timeOffset(time_t time) {
  return ( (time - (start() % period())) % period() );
}