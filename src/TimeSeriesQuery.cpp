#include "TimeSeriesQuery.h"


using namespace RTX;
using namespace std;


TimeSeriesQuery::TimeSeriesQuery() {
  _query = "select * from series_name where tag='value' and $timeFilter group by time(5m)";
}

string TimeSeriesQuery::query() {
  return _query;
}
void TimeSeriesQuery::setQuery(const std::string &query) {
  _query = query;
}


Point TimeSeriesQuery::pointBefore(time_t time) {
  auto points = _qRecord->pointsWithQuery(this->query(), TimeRange(0,time));
  return points.size() > 0 ? points.front() : Point();
}

Point TimeSeriesQuery::pointAfter(time_t time) {
  auto points = _qRecord->pointsWithQuery(this->query(), TimeRange(time,0));
  return points.size() > 0 ? points.front() : Point();
}

vector< Point > TimeSeriesQuery::points(TimeRange range) {
  auto points = _qRecord->pointsWithQuery(this->query(), range);
  return points;
}


PointRecord::_sp TimeSeriesQuery::record() {
  return _qRecord;
}

void TimeSeriesQuery::setRecord(PointRecord::_sp record) {
  auto db = dynamic_pointer_cast<DbPointRecord>(record);
  if (db) {
    _qRecord = db;
  }
}

