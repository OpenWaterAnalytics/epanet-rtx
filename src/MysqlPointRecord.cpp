//
//  MysqlPointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/warning.h>
#include <cppconn/metadata.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/statement.h>

#include "MysqlPointRecord.h"

using namespace RTX;
using namespace std;

#define RTX_CREATE_POINT_TABLE_STRING "\
    CREATE TABLE IF NOT EXISTS `points` (\
    `time` int(11) unsigned NOT NULL,\
    `series_id` int(11) NOT NULL,\
    `value` double NOT NULL,\
    `confidence` double NOT NULL,\
    `quality` tinyint(4) NOT NULL,\
    PRIMARY KEY (`series_id`,`time`)\
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;"

#define RTX_CREATE_TSKEY_TABLE_STRING "\
    CREATE TABLE IF NOT EXISTS `timeseries_meta` ( \
    `series_id`      int(11)      NOT NULL AUTO_INCREMENT, \
    `name`           varchar(255) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL, \
    `units`          varchar(12)  CHARACTER SET utf8 COLLATE utf8_bin NOT NULL, \
    `regular_period` int(11)      NOT NULL DEFAULT '0', \
    `regular_offset` int(10)      NOT NULL DEFAULT '0', \
    PRIMARY KEY (`series_id`), \
    UNIQUE KEY `name` (`name`) \
    ) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_bin AUTO_INCREMENT=1 ;"



MysqlPointRecord::MysqlPointRecord() {
  _connected = false;
}

MysqlPointRecord::~MysqlPointRecord() {
  /*
  if (_driver) {
    _driver->threadEnd();
  }
   */
}

#pragma mark - Public

void MysqlPointRecord::dbConnect() throw(RtxException) {
  
  bool databaseDoesExist = false;
  
  if (RTX_STRINGS_ARE_EQUAL(this->uid(), "") ||
      RTX_STRINGS_ARE_EQUAL(this->pwd(), "") ||
      RTX_STRINGS_ARE_EQUAL(this->db(), "") ) {
    errorMessage = "Incomplete Login";
    return;
  }
  
  
  try {
    _driver = get_driver_instance();
    _driver->threadInit();
    _mysqlCon.reset( _driver->connect(_connectionInfo.host, _connectionInfo.uid, _connectionInfo.pwd) );
    _mysqlCon->setAutoCommit(false);
    
    // test for database exists
    
    boost::shared_ptr<sql::Statement> st( _mysqlCon->createStatement() );
    sql::DatabaseMetaData *meta =  _mysqlCon->getMetaData();
    boost::shared_ptr<sql::ResultSet> results( meta->getSchemas() );
    while (results->next()) {
      std::string resultString(results->getString("TABLE_SCHEM"));
      if ( RTX_STRINGS_ARE_EQUAL(_connectionInfo.db, resultString) ) {
        databaseDoesExist = true;
        break;
      }
    }
    results->close();
    
    
    if (!databaseDoesExist) {
      string updateString;
      
      // create database
      updateString = "CREATE DATABASE ";
      updateString += _connectionInfo.db;
      st->executeUpdate(updateString);
      _mysqlCon->commit();
      _mysqlCon->setSchema(_connectionInfo.db);
      // create tables
      st->executeUpdate(RTX_CREATE_POINT_TABLE_STRING);
      st->executeUpdate(RTX_CREATE_TSKEY_TABLE_STRING);
      _mysqlCon->commit();
      //cout << "Created new Database: " << database << endl;
    }
    
    _mysqlCon->setSchema(_connectionInfo.db);
    
    
    // build the queries, since preparedStatements can't specify table names.
    //string rangeSelect = "SELECT time, value FROM " + tableName + " WHERE series_id = ? AND time > ? AND time <= ?";
    string preamble = "SELECT time, value, quality, confidence FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? AND ";
    string singleSelect = preamble + "time = ? order by time asc";
    string rangeSelect = preamble + "time >= ? AND time <= ? order by time asc";
    string nextSelect = preamble + "time > ? order by time asc LIMIT 1";
    string prevSelect = preamble + "time < ? order by time desc LIMIT 1";
    string singleInsert = "INSERT ignore INTO points (time, series_id, value, quality, confidence) SELECT ?,series_id,?,?,? FROM timeseries_meta WHERE name = ?";
    
    string firstSelectStr = "SELECT time, value, quality, confidence FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? order by time asc limit 1";
    string lastSelectStr = "SELECT time, value, quality, confidence FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? order by time desc limit 1";
    
    _rangeSelect.reset( _mysqlCon->prepareStatement(rangeSelect) );
    _singleSelect.reset( _mysqlCon->prepareStatement(singleSelect) );
    _nextSelect.reset( _mysqlCon->prepareStatement(nextSelect) );
    _previousSelect.reset( _mysqlCon->prepareStatement(prevSelect) );
    _singleInsert.reset( _mysqlCon->prepareStatement(singleInsert) );
    
    _firstSelect.reset( _mysqlCon->prepareStatement(firstSelectStr) );
    _lastSelect.reset( _mysqlCon->prepareStatement(lastSelectStr) );
    
    _connected = true;
    errorMessage = "OK";
  }
  catch (sql::SQLException &e) {
		_connected = false;
    handleException(e);
  }
}

bool MysqlPointRecord::isConnected() {
  if (!_mysqlCon || !checkConnection()) {
    return false;
  }
  boost::shared_ptr<sql::Statement> stmt;
  boost::shared_ptr<sql::ResultSet> rs;
  bool connected = false;
  try {
    stmt.reset(_mysqlCon->createStatement());
    rs.reset( stmt->executeQuery("SELECT 1") );
    if (rs->next()) {
      connected = true; // connection is valid
    }
  }
  catch (sql::SQLException &e) {
    // TODO : log the exception ...
    connected = false;
  }
  
  return connected;
}

std::string MysqlPointRecord::registerAndGetIdentifier(std::string recordName, Units dataUnits) {
  // insert the identifier, or make sure it's in the db.
  this->checkConnection();
  if (isConnected()) {
    try {
      boost::shared_ptr<sql::PreparedStatement> seriesNameStatement( _mysqlCon->prepareStatement("INSERT IGNORE INTO timeseries_meta (name,units) VALUES (?,?)") );
      seriesNameStatement->setString(1,recordName);
      seriesNameStatement->setString(2, dataUnits.unitString());
      seriesNameStatement->executeUpdate();
      _mysqlCon->commit();
    } catch (sql::SQLException &e) {
      handleException(e);
    }
  }
  
  
  DB_PR_SUPER::registerAndGetIdentifier(recordName, dataUnits);
  
  return recordName;
}


PointRecord::time_pair_t MysqlPointRecord::range(const string& id) {
  Point last;
  Point first;
  
  //cout << "range: " << id << endl;
  
  if (checkConnection()) {
    
    _firstSelect->setString(1, id);
    boost::shared_ptr<sql::ResultSet> rResults(_firstSelect->executeQuery());
    vector<Point> fsPoints = pointsFromResultSet(rResults);
    if (fsPoints.size() > 0) {
      first = fsPoints.front();
    }
    
    _lastSelect->setString(1, id);
    boost::shared_ptr<sql::ResultSet> lResults(_lastSelect->executeQuery());
    vector<Point> lsPoints = pointsFromResultSet(lResults);
    if (lsPoints.size() > 0) {
      last = lsPoints.front();
    }
  }
  return make_pair(first.time, last.time);
}

#pragma mark - simple set/get

string MysqlPointRecord::host() {
  return _connectionInfo.host;
}

void MysqlPointRecord::setHost(string host) {
  _connectionInfo.host = host;
}


string MysqlPointRecord::uid() {
  return _connectionInfo.uid;
}

void MysqlPointRecord::setUid(string uid) {
  _connectionInfo.uid = uid;
}


string MysqlPointRecord::pwd() {
  return _connectionInfo.pwd;
}

void MysqlPointRecord::setPwd(string pwd) {
  _connectionInfo.pwd = pwd;
}


string MysqlPointRecord::db() {
  return _connectionInfo.db;
}

void MysqlPointRecord::setDb(string db) {
  _connectionInfo.db = db;
}


#pragma mark - db meta

vector< pair<string, Units> > MysqlPointRecord::availableData() {
  vector< pair<string, Units> > available;
  
  if (isConnected()) {
    
    boost::shared_ptr<sql::Statement> selectNamesStatement( _mysqlCon->createStatement() );
    boost::shared_ptr<sql::ResultSet> results( selectNamesStatement->executeQuery("SELECT name,units FROM timeseries_meta WHERE 1") );
    while (results->next()) {
      // add the name to the list.
      std::string theName = results->getString("name");
      std::string theUnits = results->getString("units");
      //std::cout << "found: " << thisName << std::endl;
      available.push_back(make_pair(theName, Units::unitOfType(theUnits)));
    }
    
  }
  

  return available;
  
}


std::vector<std::string> MysqlPointRecord::identifiers() {
  std::vector<std::string> ids;
  if (!isConnected()) {
    return ids;
  }
  boost::shared_ptr<sql::Statement> selectNamesStatement( _mysqlCon->createStatement() );
  boost::shared_ptr<sql::ResultSet> results( selectNamesStatement->executeQuery("SELECT name FROM timeseries_meta WHERE 1") );
  while (results->next()) {
    // add the name to the list.
    std::string thisName = results->getString("name");
    //std::cout << "found: " << thisName << std::endl;
    ids.push_back(thisName);
  }
  return ids;
}



/*
void MysqlPointRecord::fetchRange(const std::string& id, time_t startTime, time_t endTime) {
  DbPointRecord::fetchRange(id, startTime, endTime);
}

void MysqlPointRecord::fetchNext(const std::string& id, time_t time) {
  DbPointRecord::fetchNext(id, time);
}

void MysqlPointRecord::fetchPrevious(const std::string& id, time_t time) {
  DbPointRecord::fetchPrevious(id, time);
}
 */

// select just returns the results (no caching)
std::vector<Point> MysqlPointRecord::selectRange(const std::string& id, time_t start, time_t end) {
  //cout << "mysql range: " << start << " - " << end << endl;
  
  std::vector<Point> points;
  
  if (!checkConnection()) {
    this->dbConnect();
  }
  
  if (checkConnection()) {
    _rangeSelect->setString(1, id);
    _rangeSelect->setInt(2, (int)start);
    _rangeSelect->setInt(3, (int)end);
    boost::shared_ptr<sql::ResultSet> results(_rangeSelect->executeQuery());
    points = pointsFromResultSet(results);
  }
  
  return points;
}


Point MysqlPointRecord::selectNext(const std::string& id, time_t time) {
  return selectSingle(id, time, _nextSelect);
}


Point MysqlPointRecord::selectPrevious(const std::string& id, time_t time) {
  return selectSingle(id, time, _previousSelect);
}



// insertions or alterations may choose to ignore / deny
void MysqlPointRecord::insertSingle(const std::string& id, Point point) {
  if (checkConnection()) {
    insertSingleNoCommit(id, point);
    _mysqlCon->commit();
  }
}


void MysqlPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  
  // first get a list of times already stored here, so that we don't have any overlaps.
  vector<Point> existing;
  if (checkConnection()) {
    if (points.size() > 1) {
      existing = this->selectRange(id, points.front().time, points.back().time);
    }
  }
  
  vector<time_t> timeList;
  timeList.reserve(existing.size());
  BOOST_FOREACH(Point p, existing) {
    timeList.push_back(p.time);
  }
  
  // premature optimization is the root of all evil
  bool existingRange = (timeList.size() > 0)? true : false;
  
  if (checkConnection()) {
    BOOST_FOREACH(Point p, points) {
      if (existingRange && find(timeList.begin(), timeList.end(), p.time) != timeList.end()) {
        // have it already
        continue;
      }
      else {
        insertSingleNoCommit(id, p);
      }
    }
    _mysqlCon->commit();
  }
}

void MysqlPointRecord::insertSingleNoCommit(const std::string& id, Point point) {
  _singleInsert->setInt(1, (int)point.time);
  // todo -- check this: _singleInsert->setUInt64(1, (uint64_t)time);
  _singleInsert->setDouble(2, point.value);
  _singleInsert->setInt(3, point.quality);
  _singleInsert->setDouble(4, point.confidence);
  _singleInsert->setString(5, id);
  int affected = 0;
  try {
    affected = _singleInsert->execute();
  } catch (...) {
    cout << "whoops!" << endl;
  }
  if (affected == 0) {
    // this may happen if there's already data matching this time/id primary index.
    // the insert was ignored.
    //cerr << "zero rows inserted" << endl;
  }
}

void MysqlPointRecord::removeRecord(const string& id) {
  DB_PR_SUPER::reset(id);
  if (checkConnection()) {
    string removePoints = "delete p, m from points p inner join timeseries_meta m on p.series_id=m.series_id where m.name = \"" + id + "\"";
    boost::shared_ptr<sql::Statement> removePointsStmt;
    try {
      removePointsStmt.reset( _mysqlCon->createStatement() );
      removePointsStmt->executeUpdate(removePoints);
      _mysqlCon->commit();
    } catch (sql::SQLException &e) {
      handleException(e);
    }
  }
}

void MysqlPointRecord::truncate() {
  try {
    string truncatePoints = "TRUNCATE TABLE points";
    string truncateKeys = "TRUNCATE TABLE timeseries_meta";
    
    boost::shared_ptr<sql::Statement> truncatePointsStmt, truncateKeysStmt;
    
    truncatePointsStmt.reset( _mysqlCon->createStatement() );
    truncateKeysStmt.reset( _mysqlCon->createStatement() );
    
    truncatePointsStmt->executeUpdate(truncatePoints);
    truncateKeysStmt->executeUpdate(truncateKeys);
    
    _mysqlCon->commit();
  }
  catch (sql::SQLException &e) {
    handleException(e);
  }
}




#pragma mark - Private

// caution -- result will be freed here
std::vector<Point> MysqlPointRecord::pointsFromResultSet(boost::shared_ptr<sql::ResultSet> result) {
  std::vector<Point> points;
  while (result->next()) {
    time_t time = result->getInt("time");
    double value = result->getDouble("value");
    double confidence = result->getDouble("confidence");
    int quality = result->getInt("quality");
    Point::Qual_t qtype = Point::Qual_t(quality);
    Point point(time, value, qtype, confidence);
    points.push_back(point);
  }
  return points;
}


Point MysqlPointRecord::selectSingle(const string& id, time_t time, boost::shared_ptr<sql::PreparedStatement> statement) {
  //cout << "mysql single: " << id << " -- " << time << endl;
  Point point;
  if (!_connected) {
    if(!checkConnection()) {
      return Point();
    }
  }
  statement->setString(1, id);
  statement->setInt(2, (int)time);
  boost::shared_ptr<sql::ResultSet> results(statement->executeQuery());
  vector<Point> points = pointsFromResultSet(results);
  if (points.size() > 0) {
    point = points.front();
  }
  return point;
}

bool MysqlPointRecord::checkConnection() {

  if(!_mysqlCon) {
    cerr << "mysql connection was closed. attempting to reconnect." << endl;
    this->dbConnect();
  }
  if (_mysqlCon) {
    return !(_mysqlCon->isClosed());
  }
  else {
    return false;
  }
}

void MysqlPointRecord::handleException(sql::SQLException &e) {
  /*
   The MySQL Connector/C++ throws three different exceptions:
   
   - sql::MethodNotImplementedException (derived from sql::SQLException)
   - sql::InvalidArgumentException (derived from sql::SQLException)
   - sql::SQLException (derived from std::runtime_error)
   */
  
  cerr << endl << "# ERR: DbcException in " << __FILE__ << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
  /* Use what(), getErrorCode() and getSQLState() */
  cerr << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << endl;
  
  errorMessage = std::string(e.what());
  
  if (e.getErrorCode() == 1047) {
    /*
     Error: 1047 SQLSTATE: 08S01 (ER_UNKNOWN_COM_ERROR)
     Message: Unknown command
     */
    cerr << "# ERR: Your server seems not to support PS at all because its MYSQL <4.1" << endl;
  }
  cerr << "not ok" << endl;
}







#pragma mark - Protected

std::ostream& MysqlPointRecord::toStream(std::ostream &stream) {
  stream << "Mysql Point Record (" << _connectionInfo.db << ")" << endl;
  
  
  if (!_mysqlCon || _mysqlCon->isClosed()) {
    stream << "no connection" << endl;
    return stream;
  }
    
  sql::DatabaseMetaData *meta = _mysqlCon->getMetaData();
  
	stream << "\t" << meta->getDatabaseProductName() << " " << meta->getDatabaseProductVersion() << endl;
	stream << "\tUser: " << meta->getUserName() << endl;
  
	stream << "\tDriver: " << meta->getDriverName() << " v" << meta->getDriverVersion() << endl;
  
	stream << endl;
  return stream;
}




