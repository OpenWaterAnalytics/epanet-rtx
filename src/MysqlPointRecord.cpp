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

#define RTX_CREATE_POINT_TABLE_STRING "CREATE TABLE IF NOT EXISTS `points` (`time` int(10) unsigned NOT NULL,`series_id` int(11) NOT NULL,`value` double NOT NULL,KEY `id` (`series_id`),KEY `time` (`time`)) ENGINE=InnoDB DEFAULT CHARSET=latin1;"
#define RTX_CREATE_TSKEY_TABLE_STRING "CREATE TABLE IF NOT EXISTS `timeseries_meta` (`series_id` int(11) NOT NULL AUTO_INCREMENT,`name` varchar(255) NOT NULL,PRIMARY KEY (`series_id`),UNIQUE KEY `name` (`name`)) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1;"
#define RTX_CREATE_PRETTYPRINT_STRING "CREATE VIEW `pretty_print` AS select `points`.`time` AS `time`,`timeseries_meta`.`name` AS `name`,`points`.`value` AS `value` from (`points` join `timeseries_meta` on((`points`.`series_id` = `timeseries_meta`.`series_id`))) order by `points`.`time`,`timeseries_meta`.`name`;"


MysqlPointRecord::MysqlPointRecord() {
  _connectionOk = false;
}

MysqlPointRecord::~MysqlPointRecord() {
  
}

#pragma mark - Public

void MysqlPointRecord::connect() throw(RtxException) {
  _connectionOk = false;
  
  // split the tokenized string. we're expecting something like "HOST=tcp://localhost;USER=db_user;PWD=db_pass;DB=rtx_db_name"
  std::string tokenizedString = this->connectionString();
  if (RTX_STRINGS_ARE_EQUAL(tokenizedString, "")) {
    return;
  }
  
  std::map<std::string, std::string> kvPairs;
  boost::regex kvReg("([^=]+)=([^;]+);?"); // key - value pair
  boost::sregex_iterator it(tokenizedString.begin(), tokenizedString.end(), kvReg), end;
  for ( ; it != end; ++it){
    kvPairs[(*it)[1]] = (*it)[2];
    //cout << "k: " << (*it)[1] << " // v: " << (*it)[2] << endl;
  }
  
  // if any of the keys are missing, just return.
  if (kvPairs.find("HOST") == kvPairs.end() ||
      kvPairs.find("UID") == kvPairs.end() ||
      kvPairs.find("PWD") == kvPairs.end() ||
      kvPairs.find("DB") == kvPairs.end() )
  {
    return;
    // todo -- throw something?
  }
  
  const std::string& host = kvPairs["HOST"];
  const std::string& user = kvPairs["UID"];
  const std::string& password = kvPairs["PWD"];
  const std::string& database = kvPairs["DB"];
  
  bool databaseDoesExist = false;
  try {
    _driver = get_driver_instance();
    _connection.reset( _driver->connect(host, user, password) );
    _connection->setAutoCommit(false);
    
    // test for database exists
    
    boost::shared_ptr<sql::Statement> st( _connection->createStatement() );
    sql::DatabaseMetaData *meta =  _connection->getMetaData();
    boost::shared_ptr<sql::ResultSet> results( meta->getSchemas() );
    while (results->next()) {
      std::string resultString(results->getString("TABLE_SCHEM"));
      if ( RTX_STRINGS_ARE_EQUAL(database, resultString) ) {
        databaseDoesExist = true;
        break;
      }
    }
    results->close();
    
    
    if (!databaseDoesExist) {
      string updateString;
      
      // create database
      updateString = "CREATE DATABASE ";
      updateString += database;
      st->executeUpdate(updateString);
      _connection->commit();
      _connection->setSchema(database);
      // create tables
      st->executeUpdate(RTX_CREATE_POINT_TABLE_STRING);
      st->executeUpdate(RTX_CREATE_TSKEY_TABLE_STRING);
      st->executeUpdate(RTX_CREATE_PRETTYPRINT_STRING);
      _connection->commit();
      cout << "Created new Database: " << database << endl;
    }
    
    _connection->setSchema(database);
    
    
    // build the queries, since preparedStatements can't specify table names.
    //string rangeSelect = "SELECT time, value FROM " + tableName + " WHERE series_id = ? AND time > ? AND time <= ?";
    string preamble = "SELECT time, value FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? AND ";
    string singleSelect = preamble + "time = ?";
    string rangeSelect = preamble + "time >= ? AND time <= ?";
    string nextSelect = preamble + "time > ? LIMIT 1";
    string prevSelect = preamble + "time < ? LIMIT 1";
    string singleInsert = "INSERT INTO points (time, series_id, value) SELECT ?,series_id,? FROM timeseries_meta WHERE name = ?";
    
    string firstSelectStr = "SELECT time, value FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? order by time asc limit 1";
    string lastSelectStr = "SELECT time, value FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? order by time desc limit 1";
    
    _rangeSelect.reset( _connection->prepareStatement(rangeSelect) );
    _singleSelect.reset( _connection->prepareStatement(singleSelect) );
    _nextSelect.reset( _connection->prepareStatement(nextSelect) );
    _previousSelect.reset( _connection->prepareStatement(prevSelect) );
    _singleInsert.reset( _connection->prepareStatement(singleInsert) );
    
    _firstSelect.reset( _connection->prepareStatement(firstSelectStr) );
    _lastSelect.reset( _connection->prepareStatement(lastSelectStr) );
    
    // if we made it this far...
    _connectionOk = true;
    _name = database;
  }
  catch (sql::SQLException &e) {
		handleException(e);
  }
}

bool MysqlPointRecord::isConnected() {
  if (!_connection || !_connectionOk) {
    return false;
  }
  boost::shared_ptr<sql::Statement> stmt;
  boost::shared_ptr<sql::ResultSet> rs;
  bool connected = false;
  try {
    stmt.reset(_connection->createStatement());
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

std::string MysqlPointRecord::registerAndGetIdentifier(std::string recordName) {
  // insert the identifier, or make sure it's in the db.
  try {
    boost::shared_ptr<sql::PreparedStatement> seriesNameStatement( _connection->prepareStatement("INSERT IGNORE INTO timeseries_meta (name) VALUES (?)") );
    seriesNameStatement->setString(1,recordName);
    seriesNameStatement->executeUpdate();
    _connection->commit();
  } catch (sql::SQLException &e) {
    handleException(e);
	}
  
  PointRecord::registerAndGetIdentifier(recordName);
  
  return recordName;
}

std::vector<std::string> MysqlPointRecord::identifiers() {
  std::vector<std::string> ids;
  if (!isConnected()) {
    return ids;
  }
  boost::shared_ptr<sql::Statement> selectNamesStatement( _connection->createStatement() );
  boost::shared_ptr<sql::ResultSet> results( selectNamesStatement->executeQuery("SELECT name FROM timeseries_meta WHERE 1") );
  while (results->next()) {
    // add the name to the list.
    std::string thisName = results->getString("name");
    //std::cout << "found: " << thisName << std::endl;
    ids.push_back(thisName);
  }
  return ids;
}


bool MysqlPointRecord::isPointAvailable(const string& identifier, time_t time) {
  if (_cachedPoint.time() == time && _cachedPointId == identifier) {
    return true;
  }
  if (PointRecord::isPointAvailable(identifier, time)) {
    return true;
  }
  
  Point point = selectSingle(identifier, time, _singleSelect);
  if (point.isValid()) {
    return true;
  }
  return false;
}
Point MysqlPointRecord::point(const string& identifier, time_t time) {
  
  if (time == _cachedPoint.time() && identifier == _cachedPointId) {
    return _cachedPoint;
  }
  
  Point thePoint;
  
  // see if the requested point is within my cache
  thePoint = PointRecord::point(identifier, time);
  if (!thePoint.isValid()) {
    thePoint = selectSingle(identifier, time, _singleSelect);
    PointRecord::addPoint(identifier, thePoint);
  }
  
  
  return thePoint;
}
Point MysqlPointRecord::pointBefore(const string& identifier, time_t time) {
  Point point = selectSingle(identifier, time, _previousSelect);
  if (point.isValid()) {
    return point;
  }
  else {
    // todo - throw something?
    return point;
  }
}
Point MysqlPointRecord::pointAfter(const string& identifier, time_t time) {
  Point point = selectSingle(identifier, time, _nextSelect);
  if (point.isValid()) {
    return point;
  }
  else {
    // todo - throw something?
    //std::cerr << "could not find point after " << time << std::endl;
    return point;
  }
}

void MysqlPointRecord::addPoint(const string& identifier, Point point) {
  time_t time = point.time();
  double value = point.value();
  insertSingle(identifier, time, value);
}

void MysqlPointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  BOOST_FOREACH(Point point, points) {
    addPoint(identifier, point);
  }
}

void MysqlPointRecord::reset() {
  PointRecord::reset();
  try {
    string truncatePoints = "TRUNCATE TABLE points";
    string truncateKeys = "TRUNCATE TABLE timeseries_meta";
    
    boost::shared_ptr<sql::Statement> truncatePointsStmt, truncateKeysStmt;
    
    truncatePointsStmt.reset( _connection->createStatement() );
    truncateKeysStmt.reset( _connection->createStatement() );
    
    truncatePointsStmt->executeUpdate(truncatePoints);
    //truncateKeysStmt->executeUpdate(truncateKeys);
    
    _connection->commit();
  }
  catch (sql::SQLException &e) {
    handleException(e);
  }
}

void MysqlPointRecord::reset(const string& identifier) {
  PointRecord::reset(identifier);
  string removePoints = "delete p, m from points p inner join timeseries_meta m on p.series_id=m.series_id where m.name = \"" + identifier + "\"";
  boost::shared_ptr<sql::Statement> removePointsStmt;
  try {
    removePointsStmt.reset( _connection->createStatement() );
    removePointsStmt->executeUpdate(removePoints);
    _connection->commit();
  } catch (sql::SQLException &e) {
    handleException(e);
  }
}

#pragma mark - Protected

std::ostream& MysqlPointRecord::toStream(std::ostream &stream) {
  stream << "Mysql Point Record (" << _name << ")" << endl;
  
  
  if (_connection -> isClosed()) {
    stream << "no connection" << endl;
    return stream;
  }
    
  sql::DatabaseMetaData *meta = _connection->getMetaData();
  
	stream << "\t" << meta->getDatabaseProductName() << " " << meta->getDatabaseProductVersion() << endl;
	stream << "\tUser: " << meta->getUserName() << endl;
  
	stream << "\tDriver: " << meta->getDriverName() << " v" << meta->getDriverVersion() << endl;
  
	stream << endl;
  return stream;
}

Point MysqlPointRecord::firstPoint(const string &id) {
  Point point;
  _firstSelect->setString(1, id);
  boost::shared_ptr<sql::ResultSet> result( _firstSelect->executeQuery() );
  if( result && result->next() ) {
    time_t rowTime = result->getInt("time");
    double rowValue = result->getDouble("value");
    point = Point(rowTime, rowValue);
  }
  return point;
}

Point MysqlPointRecord::lastPoint(const string &id) {
  Point point;
  _lastSelect->setString(1, id);
  boost::shared_ptr<sql::ResultSet> result( _lastSelect->executeQuery() );
  if( result->next() ) {
    time_t rowTime = result->getInt("time");
    double rowValue = result->getDouble("value");
    point = Point(rowTime, rowValue);
  }
  return point;
}


#pragma mark - Private

void MysqlPointRecord::insertSingle(const string& identifier, time_t time, double value) {
  // "INSERT INTO points
  _singleInsert->setInt(1, (int)time);
  // todo -- check this: _singleInsert->setUInt64(1, (uint64_t)time);
  _singleInsert->setDouble(2, value);
  _singleInsert->setString(3, identifier);
  int affected = _singleInsert->executeUpdate();
  if (affected == 0) {
    // throw something?
    cerr << "zero rows inserted" << endl;
  }
  _connection->commit();
}

Point MysqlPointRecord::selectSingle(const string& identifier, time_t time, boost::shared_ptr<sql::PreparedStatement> statement) {
  
  Point point;
  statement->setString(1, identifier);
  statement->setInt(2, (int)time);
  boost::shared_ptr<sql::ResultSet> result( statement->executeQuery() );
  if( result->next() ) {
    double rowValue = result->getDouble("value");
    time_t rowTime = result->getInt("time");
    point = Point(rowTime, rowValue);
  }
  return point;
}

std::vector<Point> MysqlPointRecord::selectRange(const string& identifier, time_t start, time_t end) {
  // "SELECT time, value FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? AND time > ? AND time <= ?";
  std::vector<Point> points;
  _rangeSelect->setString(1, identifier);
  _rangeSelect->setInt(2, (int)start);
  _rangeSelect->setInt(3, (int)end);
  boost::shared_ptr<sql::ResultSet> result( _rangeSelect->executeQuery() );
  while (result->next()) {
    time_t time = result->getInt("time");
    double value = result->getDouble("value");
    Point point(time, value);
    points.push_back(point);
  }
  
  return points;
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
  
  if (e.getErrorCode() == 1047) {
    /*
     Error: 1047 SQLSTATE: 08S01 (ER_UNKNOWN_COM_ERROR)
     Message: Unknown command
     */
    cerr << "# ERR: Your server seems not to support PS at all because its MYSQL <4.1" << endl;
  }
  cerr << "not ok" << endl;
  _connectionOk = false;
}








