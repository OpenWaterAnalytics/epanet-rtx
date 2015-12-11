#include "TimeSeriesDuplicator.h"

#include "TimeSeriesFilter.h"

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>

using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

using namespace std;
using namespace RTX;

TimeSeriesDuplicator::TimeSeriesDuplicator() {
  _isRunning = false;
  _shouldRun = true;
  _pctCompleteFetch = 0;
  logLevel = RTX_DUPLICATOR_LOGLEVEL_ERROR;
}

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
  this->_shouldRun = true;
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
  
  
  stringstream s;
  s << "Starting fetch: freq-" << frequency << " win-" << fetchWindow;
  this->_logLine(s.str(),RTX_DUPLICATOR_LOGLEVEL_INFO);
  
  
  
  bool saveMetrics = true;
  TimeSeries::_sp metricsPointCount( new TimeSeries );
  metricsPointCount->setUnits(RTX_DIMENSIONLESS);
  metricsPointCount->setName("metrics_point_count");
  TimeSeries::_sp metricsTimeElapased( new TimeSeries );
  metricsTimeElapased->setUnits(RTX_SECOND);
  metricsTimeElapased->setName("metrics_time_elapsed");
  if (saveMetrics) {
    metricsPointCount->setRecord(_destinationRecord);
    metricsTimeElapased->setRecord(_destinationRecord);
  }
  
  
  while (_shouldRun) {
    _isRunning = true;
    
    pair<time_t,int> fetchRes = this->_fetchAll(nextFetch - fetchWindow, nextFetch);
    time_t fetchDuration = fetchRes.first;
    int nPoints = fetchRes.second;
    if (saveMetrics) {
      metricsPointCount->insert(Point(time(NULL), (double)nPoints));
      metricsTimeElapased->insert(Point(time(NULL), (double)fetchDuration));
    }
    
    
    nextFetch += frequency;
    time_t waitLength = nextFetch - time(NULL);
    if (waitLength < 0) {
      waitLength = 0;
    }
    stringstream ss;
    char *tstr = asctime(localtime(&nextFetch));
    tstr[24] = '\0';
    ss << "Fetch: (" << tstr << ") took " << fetchDuration << " seconds." << "\n" << "Waiting for " << waitLength << " seconds";
    this->_logLine(ss.str(),RTX_DUPLICATOR_LOGLEVEL_INFO);
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

void TimeSeriesDuplicator::runRetrospective(time_t start, time_t chunkSize, time_t rateLimit) {
  bool saveMetrics = true;
  TimeSeries::_sp metricsPointCount( new TimeSeries );
  metricsPointCount->setUnits(RTX_DIMENSIONLESS);
  metricsPointCount->setName("metrics_point_count");
  TimeSeries::_sp metricsTimeElapased( new TimeSeries );
  metricsTimeElapased->setUnits(RTX_SECOND);
  metricsTimeElapased->setName("metrics_time_elapsed");
  if (saveMetrics) {
    metricsPointCount->setRecord(_destinationRecord);
    metricsTimeElapased->setRecord(_destinationRecord);
  }
  
  time_t nextFetch = start + chunkSize;
  _shouldRun = true;
  
  stringstream s;
  s << "Starting catchup from " << start << " in " << chunkSize << "second chunks";
  this->_logLine(s.str(),RTX_DUPLICATOR_LOGLEVEL_INFO);
  
  bool inThePast = start < time(NULL);
  
  while (_shouldRun && inThePast) {
    time_t thisLoop = time(NULL);
    _isRunning = true;
    time_t fetchEndTime = nextFetch;
    if (nextFetch > thisLoop) {
      fetchEndTime = thisLoop;
      inThePast = false;
    }
    
    pair<time_t,int> fetchRes = this->_fetchAll(nextFetch - chunkSize, fetchEndTime);
    time_t fetchDuration = fetchRes.first;
    int nPoints = fetchRes.second;
    
    nextFetch += chunkSize;
    
    char *tstr = asctime(localtime(&fetchEndTime));
    tstr[24] = '\0';
    stringstream ss;
    ss << "RETROSPECTIVE Fetch: (" << tstr << ") took " << fetchDuration << " seconds.";
    this->_logLine(ss.str(), RTX_DUPLICATOR_LOGLEVEL_INFO);
    if (rateLimit > 0 && inThePast) {
      ss.clear();
      ss << "\nRate Limited - Waiting for " << rateLimit << " seconds";
      this->_logLine(ss.str(), RTX_DUPLICATOR_LOGLEVEL_VERBOSE);
    }
    
    if (saveMetrics) {
      metricsPointCount->insert(Point(time(NULL), (double)nPoints));
      metricsTimeElapased->insert(Point(time(NULL), (double)fetchDuration));
    }
    
    time_t wakeup = time(NULL) + rateLimit;
    while (_shouldRun && rateLimit > 0 && wakeup > time(NULL) && inThePast) {
      boost::this_thread::sleep_for(boost::chrono::seconds(1));
    }
  }
  
  // outside the loop means it was cancelled by user.
  _isRunning = false;
  
}


std::pair<time_t,int> TimeSeriesDuplicator::_fetchAll(time_t start, time_t end) {
  time_t fStart = time(NULL);
  int nPoints = 0;
  _pctCompleteFetch = 0.;
  size_t nSeries = _destinationSeries.size();
  BOOST_FOREACH(TimeSeries::_sp ts, _destinationSeries) {
    if (_shouldRun) {
      ts->resetCache();
      TimeSeries::PointCollection pc = ts->pointCollection(TimeRange(start, end));
      stringstream tsSS;
      tsSS << ts->name() << " : " << pc.count() << " points (max:" << pc.max() << " min:" << pc.min() << " avg:" << pc.mean() << ")";
      this->_logLine(tsSS.str(),RTX_DUPLICATOR_LOGLEVEL_VERBOSE);
      _pctCompleteFetch += 1./(double)nSeries;
      nPoints += pc.count();
    }
    
  }
  _pctCompleteFetch = 1.;
  return make_pair(time(NULL) - fStart, nPoints);
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

void TimeSeriesDuplicator::_logLine(const std::string& line, int level) {
  if (this->logLevel < level) {
    return;
  }
  if (_loggingFn == NULL) {
    return;
  }
  string myLine(line);
  if (_loggingFn != NULL) {
    size_t loc = myLine.find("\n");
    if (loc+1 != myLine.length()) {
      myLine += "\n";
    }
    const char *msg = myLine.c_str();
    _loggingFn(msg);
  }
}

