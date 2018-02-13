//
//  AutoRunner.cpp
//  LINK-service
//
//  Created by sam hatchett on 2/12/18.
//

#include "AutoRunner.hpp"

using namespace RTX;
using namespace std;

AutoRunner::AutoRunner() {
  _is_running_token = false;
  _cancellation_token = false;
  setLogging([](string msg){
    cout << msg << endl;
  }, RTX_AUTORUNNER_LOGLEVEL_VERBOSE);
}

void AutoRunner::setSeries(std::vector<TimeSeries::_sp> series) {
  _series.clear();
  for(auto s : series) {
    TsEntry e;
    e.series = s;
    e.lastGood = 0; // never
    _series.push_back(e);
  }
}

void AutoRunner::setParams(bool smartQueries, int maxWindowSeconds, int frequencySeconds, int throttleSeconds) {
  _smart = smartQueries;
  _window = maxWindowSeconds;
  _freq = frequencySeconds;
  _throttle = throttleSeconds;
}

void AutoRunner::run(time_t since) {
  
  for(auto &e : _series) {
    if (e.lastGood == 0) { // never fetched.
      e.lastGood = since;
    }
  }
  
  _task = std::async(launch::async, [&,since]() -> void {
    this->_runLoop(_cancellation_token);
  });
  
}

void AutoRunner::cancel() {
  _logFn("Cancellation token recieved");
  _cancellation_token = true;
}
bool AutoRunner::isRunning() {
  return _is_running_token ? true : false;
}

void AutoRunner::setLogging(std::function<void (std::string)> fn, int level) {
  _logFn = fn;
  _logLevel = level;
}

void AutoRunner::_log(std::string msg, int msgLevel) {
  if (msgLevel <= _logLevel) {
    _logFn(msg);
  }
}

void AutoRunner::_runLoop(std::atomic_bool &cancel) {
  _is_running_token = true;
  while (!cancel) {
    time_t tick = time(NULL);
    
    for(auto &e : _series) {
      // might be doing a backfill,
      // if the last-good is older than window.
      while (tick - e.lastGood > (_window + _freq)) {
        // this is the backfill loop.
        // just query with window-width ranges until queries are all caught-up.
        // ignore whether there's actually data there.
        // this is where data can be lost if it's older than window.
        // the (window+freq) test above means that, in a data-sparse environment
        // or in a failure mode, we would expect this staleness, so ignore it and move on.
        // if no points are returned in normal mode, then the next time we hit this scope,
        // it will be the last chance data has to get captured.
        time_t qEnd = tick - _window;
        if (qEnd - e.lastGood > _window) {
          qEnd = e.lastGood + _window;
        }
        e.series->points(TimeRange(e.lastGood, qEnd));
        e.lastGood = qEnd; // increment the backfill window
      }
      
      // ok, now we are in "normal" querying mode.
      // get a range of data.
      auto points = e.series->points(TimeRange(e.lastGood, tick));
      
      if (_smart) {
        // smart queries means keep track of the last-known good point
        // this also means no overlap between subsequent queries.
        if (points.size() > 0) {
          auto lastPoint = points.rbegin();
          e.lastGood = lastPoint->time;
        }
      }
      else {
        // dumb queries: each query is window-length long.
        // this allows for some overlap (tunable parameter)
        e.lastGood = (tick + _freq) - _window;
      }
      
      
    } // for series
    
    // wait for another cycle? check often.
    while (!cancel && tick + _freq > time(NULL)) {
      this_thread::sleep_for(chrono::milliseconds(200));
    }
    
  } // while !cancel
  
  _is_running_token = false;
  _cancellation_token = false;
  
}





