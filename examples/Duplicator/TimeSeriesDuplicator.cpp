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
  if (!_destinationRecord) {
    return;
  }
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

time_t TimeSeriesDuplicator::fetchWindow() {
  return _fetchWindow;
}
void TimeSeriesDuplicator::setFetchWindow(time_t seconds) {
  scoped_lock<boost::signals2::mutex> lock(_mutex);
  _fetchWindow = seconds;
}

time_t TimeSeriesDuplicator::fetchFrequency() {
  return _fetchFrequency;
}
void TimeSeriesDuplicator::setFetchFrequency(time_t seconds) {
  scoped_lock<boost::signals2::mutex> lock(_mutex);
  _fetchFrequency = seconds;
}

void TimeSeriesDuplicator::run() {
  
}
void TimeSeriesDuplicator::stop() {
  
}
void TimeSeriesDuplicator::runWithRetrospective(time_t start, time_t chunkSize) {
  
}
bool TimeSeriesDuplicator::isRunning() {
  return false;
}
double TimeSeriesDuplicator::pctCompleteFetch() {
  return 0.;
}


// boost::this_thread::sleep_for(boost::posix_time::seconds(60));

