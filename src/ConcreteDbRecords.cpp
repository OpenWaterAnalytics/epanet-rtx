#include "ConcreteDbRecords.h"

// adaptors
#include "SqliteAdapter.h"
#include "PiAdapter.h"
#include "InfluxAdapter.h"
#include "OdbcAdapter.h"

using namespace RTX;
using namespace std;

/***************************************************************************************/

SqlitePointRecord::SqlitePointRecord() {
  _adapter = new SqliteAdapter(_errCB);
}
SqlitePointRecord::~SqlitePointRecord() {
  delete _adapter;
}

/***************************************************************************************/

PiPointRecord::PiPointRecord() {
  _adapter = new PiAdapter(_errCB);
}
PiPointRecord::~PiPointRecord() {
  delete _adapter;
}
void PiPointRecord::setTagSearchPath(const std::string& path) {
  ((PiAdapter*)_adapter)->tagSearchPath = path;
}
std::string PiPointRecord::tagSearchPath() {
  return ((PiAdapter*)_adapter)->tagSearchPath;
}

/***************************************************************************************/


InfluxDbPointRecord::InfluxDbPointRecord() {
  _adapter = new InfluxTcpAdapter(_errCB);
}
InfluxDbPointRecord::~InfluxDbPointRecord() {
  delete _adapter;
}

void InfluxDbPointRecord::sendInfluxString(time_t time, const string& seriesId, const string& values) {
  ((InfluxAdapter*)_adapter)->sendInfluxString(time, seriesId, values);
}

/***************************************************************************************/

InfluxUdpPointRecord::InfluxUdpPointRecord() {
  _adapter = new InfluxUdpAdapter(_errCB);
}
InfluxUdpPointRecord::~InfluxUdpPointRecord() {
  delete _adapter;
}
void InfluxUdpPointRecord::sendInfluxString(time_t time, const string& seriesId, const string& values) {
  ((InfluxAdapter*)_adapter)->sendInfluxString(time, seriesId, values);
}

/***************************************************************************************/


OdbcPointRecord::OdbcPointRecord() {
  _adapter = new OdbcAdapter(_errCB);
}
OdbcPointRecord::~OdbcPointRecord() {
  delete _adapter;
}

std::list<std::string> OdbcPointRecord::driverList() {
  return OdbcAdapter::listDrivers();
}

void OdbcPointRecord::setTimeFormat(PointRecordTime::time_format_t timeFormat) {
  ((OdbcAdapter*)_adapter)->setTimeFormat(timeFormat);
}
PointRecordTime::time_format_t OdbcPointRecord::timeFormat() {
  return ((OdbcAdapter*)_adapter)->timeFormat();
}

std::string OdbcPointRecord::timeZoneString() {
  return ((OdbcAdapter*)_adapter)->timeZoneString();
}
void OdbcPointRecord::setTimeZoneString(const std::string& tzStr) {
  ((OdbcAdapter*)_adapter)->setTimeZoneString(tzStr);
}

std::string OdbcPointRecord::driver() {
  return ((OdbcAdapter*)_adapter)->driver();
}
void OdbcPointRecord::setDriver(const std::string& driver) {
  ((OdbcAdapter*)_adapter)->setDriver(driver);
}

std::string OdbcPointRecord::metaQuery() {
  return ((OdbcAdapter*)_adapter)->metaQuery();
}
void OdbcPointRecord::setMetaQuery(const std::string& meta) {
  ((OdbcAdapter*)_adapter)->setMetaQuery(meta);
}

std::string OdbcPointRecord::rangeQuery() {
  return ((OdbcAdapter*)_adapter)->rangeQuery();
}
void OdbcPointRecord::setRangeQuery(const std::string& range) {
  ((OdbcAdapter*)_adapter)->setRangeQuery(range);
}


