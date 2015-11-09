#include "TimeSeriesDuplicator.h"

#include "TimeSeriesFilter.h"

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>

using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

using namespace std;
using namespace RTX;


PointRecord::_sp TimeSeriesDuplicator::destinationRecord() {
  return _destinationRecord;
}

void TimeSeriesDuplicator::setDestinationRecord(PointRecord::_sp record) {
  _destinationRecord = record;
  this->_refreshDestinations();
}

std::list<TimeSeries::_sp> TimeSeriesDuplicator::series() {
  return _sourceSeries;
}
void TimeSeriesDuplicator::setSeries(std::list<TimeSeries::_sp> series) {
  this->stop();
  _sourceSeries = series;
  this->_refreshDestinations();
}

void TimeSeriesDuplicator::_refreshDestinations() {
  _destinationSeries.clear();
  
  BOOST_FOREACH(TimeSeries::_sp source, _sourceSeries) {
    // make a simple modular ts
    TimeSeriesFilter::_sp mod( new TimeSeriesFilter() );
    mod->setSource(source);
    mod->setName(source->name());
    mod->setRecord(_destinationRecord);
    _destinationSeries.push_back(mod);
  }
  
}

void TimeSeriesDuplicator::run(time_t fetchWindow, time_t frequency) {
  _shouldRun = true;
  time_t nextFetch = time(NULL);
  
  while (_shouldRun) {
    _isRunning = true;
    time_t loopStarted = time(NULL);
    
    _pctCompleteFetch = 0.;
    size_t nSeries = _destinationSeries.size();
    BOOST_FOREACH(TimeSeries::_sp ts, _destinationSeries) {
      if (_shouldRun) {
        ts->resetCache();
        TimeSeries::PointCollection pc = ts->pointCollection(TimeRange(nextFetch - fetchWindow, nextFetch));
        stringstream tsSS;
        tsSS << ts->name() << " : " << pc.count() << " points (max:" << pc.max() << " min:" << pc.min() << " avg:" << pc.mean() << ")";
        this->_logLine(tsSS.str());
        _pctCompleteFetch += 1./(double)nSeries;
      }
      
    }
    _pctCompleteFetch = 1.;
    time_t postFetch = time(NULL);
    time_t fetchDuration = postFetch - loopStarted;
    nextFetch += frequency;
    time_t waitLength = nextFetch - time(NULL);
    if (waitLength < 0) {
      waitLength = 0;
    }
    stringstream ss;
    ss << "Fetch took " << fetchDuration << " seconds. Waiting for " << waitLength << " seconds";
    this->_logLine(ss.str());
    while (_shouldRun && nextFetch > time(NULL)) {
      boost::this_thread::sleep_for(boost::chrono::seconds(1));
    }
  }
  
  // outside the loop means it was cancelled by user.
  _isRunning = false;
}
void TimeSeriesDuplicator::stop() {
  _shouldRun = false;
}
void TimeSeriesDuplicator::runRetrospective(time_t start, time_t chunkSize) {
  
}
bool TimeSeriesDuplicator::isRunning() {
  return _isRunning;
}
double TimeSeriesDuplicator::pctCompleteFetch() {
  return _pctCompleteFetch;
}



void TimeSeriesDuplicator::setLoggingFunction(RTX_Duplicator_Logging_Callback_Block fn) {
  _loggingFn = fn;
}

TimeSeriesDuplicator::RTX_Duplicator_Logging_Callback_Block TimeSeriesDuplicator::loggingFunction() {
  return _loggingFn;
}

void TimeSeriesDuplicator::_logLine(const std::string& line) {
  if (_loggingFn == NULL) {
    return;
  }
  string myLine(line);
  if (_loggingFn != NULL) {
    size_t loc = myLine.find("\n");
    if (loc == string::npos) {
      myLine += "\n";
    }
    const char *msg = myLine.c_str();
    _loggingFn(msg);
  }
}

