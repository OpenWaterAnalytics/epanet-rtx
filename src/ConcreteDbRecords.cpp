#include "ConcreteDbRecords.h"

// adaptors
#include "SqliteAdapter.h"
//#include "PiAdapter.h"
#include "InfluxAdapter.h"

using namespace RTX;
using namespace std;

/***************************************************************************************/

SqlitePointRecord::SqlitePointRecord() {
  _adapter = new SqliteAdapter(_errCB);
}
SqlitePointRecord::~SqlitePointRecord() {
  delete _adapter;
}

std::string SqlitePointRecord::basePath() {
  return ((SqliteAdapter*)_adapter)->basePath;
}
void SqlitePointRecord::setBasePath(const std::string& path) {
  ((SqliteAdapter*)_adapter)->basePath = path;
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
