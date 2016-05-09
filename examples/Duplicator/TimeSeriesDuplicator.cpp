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
  logLevel = RTX_DUPLICATOR_LOGLEVEL_INFO;
}

PointRecord::_sp TimeSeriesDuplicator::destinationRecord() {
  return _destinationRecord;
}

void TimeSeriesDuplicator::setDestinationRecord(PointRecord::_sp record) {
  {
    scoped_lock<boost::signals2::mutex> mx(_mutex);
    _destinationRecord = record;
  }
  this->_refreshDestinations();
}

std::list<TimeSeries::_sp> TimeSeriesDuplicator::series() {
  return _sourceSeries;
}
void TimeSeriesDuplicator::setSeries(std::list<TimeSeries::_sp> series) {
  this->stop();
  {
    scoped_lock<boost::signals2::mutex> mx(_mutex);
    _sourceSeries = series;
  }
  this->_refreshDestinations();
  this->_shouldRun = true;
}

void TimeSeriesDuplicator::_refreshDestinations() {
  {
    scoped_lock<boost::signals2::mutex> mx(_mutex);
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
}



void TimeSeriesDuplicator::catchUpAndRun(time_t fetchWindow, time_t frequency, time_t backfill, time_t chunkSize, time_t rateLimit) {
  boost::thread t(&TimeSeriesDuplicator::_catchupLoop, this, fetchWindow, frequency, backfill, chunkSize, rateLimit);
  _dupeBackground.swap(t);
}

void TimeSeriesDuplicator::_catchupLoop(time_t fetchWindow, time_t frequency, time_t backfill, time_t chunkSize, time_t rateLimit) {
  _shouldRun = true;
  time_t backfillStart = time(NULL) - backfill;
  this->_backfillLoop(backfillStart, chunkSize);
  if (!_shouldRun) {
    return;
  }
  this->_dupeLoop(fetchWindow, frequency);
  
}

void TimeSeriesDuplicator::run(time_t fetchWindow, time_t frequency) {
  
  boost::thread t(&TimeSeriesDuplicator::_dupeLoop, this, fetchWindow, frequency);
  _dupeBackground.swap(t);
  
}

void TimeSeriesDuplicator::wait() {
  if (_dupeBackground.joinable()) {
    _dupeBackground.join();
  }
}

void TimeSeriesDuplicator::_dupeLoop(time_t win, time_t freq) {
  _shouldRun = true;
  
  time_t nextFetch = time(NULL);
  
  
  stringstream s;
  s << "Starting fetch: freq-" << freq << " win-" << win;
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
    
    pair<time_t,int> fetchRes = this->_fetchAll(nextFetch - win, nextFetch);
    time_t fetchDuration = fetchRes.first;
    int nPoints = fetchRes.second;
    time_t now = time(NULL);
    if (saveMetrics) {
      metricsPointCount->insert(Point(now, (double)nPoints));
      metricsTimeElapased->insert(Point(now, (double)fetchDuration));
    }
    
    
    nextFetch += freq;
    
    // edge case: sometimes system clock hasn't been set (maybe no ntp response yet)
    if (nextFetch < now) {
      // check how far off we are?
      // arbitrarily, let us be off by 5 windows...
      if (nextFetch + (5 * freq) < now) {
        nextFetch = now; // just skip a bunch and get us current.
        this->_logLine("Skipping an interval due to too much lag", RTX_DUPLICATOR_LOGLEVEL_WARN);
      }
    }
    
    stringstream ss;
    char *tstr = asctime(localtime(&nextFetch));
    tstr[24] = '\0';
    while (_shouldRun && nextFetch > time(NULL)) {
      time_t waitLength = nextFetch - time(NULL);
      ss.str("");
      ss << "Fetch: (" << tstr << ") took " << fetchDuration << " seconds." << "\n" << "Will fire again in " << waitLength << " seconds";
      this->_logLine(ss.str(),RTX_DUPLICATOR_LOGLEVEL_INFO);
      boost::this_thread::sleep_for(boost::chrono::seconds(1));
    }
  }
  
  // outside the loop means it was cancelled by user.
  _isRunning = false;
  
  if (!_shouldRun) {
    _logLine("Stopped Duplication by User Request", RTX_DUPLICATOR_LOGLEVEL_INFO);
  }
  
}

void TimeSeriesDuplicator::stop() {
  _shouldRun = false;
}

void TimeSeriesDuplicator::runRetrospective(time_t start, time_t chunkSize, time_t rateLimit) {
  
  boost::thread t(&TimeSeriesDuplicator::_backfillLoop, this, start, chunkSize, rateLimit);
  _dupeBackground.swap(t);
  
}

void TimeSeriesDuplicator::_backfillLoop(time_t start, time_t chunk, time_t rateLimit) {
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
  
  time_t nextFetch = start + chunk;
  _shouldRun = true;
  
  stringstream s;
  s << "Starting catchup from " << start << " in " << chunk << "second chunks";
  this->_logLine(s.str(),RTX_DUPLICATOR_LOGLEVEL_INFO);
  
  bool inThePast = start < time(NULL);
  
  _pctCompleteFetch = 0.;
  
  while (_shouldRun && inThePast) {
    time_t thisLoop = time(NULL);
    _isRunning = true;
    time_t fetchEndTime = nextFetch;
    if (nextFetch > thisLoop) {
      fetchEndTime = thisLoop;
      inThePast = false;
    }
    
    pair<time_t,int> fetchRes = this->_fetchAll(nextFetch - chunk, fetchEndTime, false);
    time_t fetchDuration = fetchRes.first;
    int nPoints = fetchRes.second;
    
    nextFetch += chunk;
    
    _pctCompleteFetch = (double)(nextFetch - start) / (double)(thisLoop - start) ;
    
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
  
  if (!_shouldRun) {
    _logLine("Stopped Duplication by User Request", RTX_DUPLICATOR_LOGLEVEL_INFO);
  }
  
}


std::pair<time_t,int> TimeSeriesDuplicator::_fetchAll(time_t start, time_t end, bool updatePctComplete) {
  scoped_lock<boost::signals2::mutex> mx(_mutex);
  time_t fStart = time(NULL);
  int nPoints = 0;
  if (updatePctComplete) {
    _pctCompleteFetch = 0.;
  }
  size_t nSeries = _destinationSeries.size();
  BOOST_FOREACH(TimeSeries::_sp ts, _destinationSeries) {
    if (_shouldRun) {
      ts->resetCache();
      TimeSeries::PointCollection pc = ts->pointCollection(TimeRange(start, end));
      stringstream tsSS;
      tsSS << ts->name() << " : " << pc.count() << " points (max:" << pc.max() << " min:" << pc.min() << " avg:" << pc.mean() << ")";
      this->_logLine(tsSS.str(),RTX_DUPLICATOR_LOGLEVEL_VERBOSE);
      if (updatePctComplete) {
        _pctCompleteFetch += 1./(double)nSeries;
      }
      nPoints += pc.count();
    }
    
  }
  if (updatePctComplete) {
    _pctCompleteFetch = 1.;
  }
  return make_pair(time(NULL) - fStart, nPoints);
}


bool TimeSeriesDuplicator::isRunning() {
  return _isRunning;
}
double TimeSeriesDuplicator::pctCompleteFetch() {
  return _pctCompleteFetch;
}



void TimeSeriesDuplicator::setLoggingFunction(RTX_Duplicator_log_callback fn) {
  _logFn = fn;
}

TimeSeriesDuplicator::RTX_Duplicator_log_callback TimeSeriesDuplicator::loggingFunction() {
  return _logFn;
}

void TimeSeriesDuplicator::_logLine(const std::string& line, int level) {
  if (this->logLevel < level) {
    return;
  }
  if (_logFn == NULL) {
    return;
  }
  string myLine;
  
  switch (level) {
    case RTX_DUPLICATOR_LOGLEVEL_WARN:
      myLine += "WARN: ";
      break;
    case RTX_DUPLICATOR_LOGLEVEL_ERROR:
      myLine += "ERROR: ";
      break;
    case RTX_DUPLICATOR_LOGLEVEL_INFO:
    case RTX_DUPLICATOR_LOGLEVEL_VERBOSE:
      myLine += "INFO: ";
      break;
    default:
      break;
  }
  
  myLine += line;
  if (_logFn != NULL) {
    size_t loc = myLine.find("\n");
    if (loc+1 != myLine.length()) {
      myLine += "\n";
    }
    const char *msg = myLine.c_str();
    _logFn(msg);
  }
}

