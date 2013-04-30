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
    PRIMARY KEY (`time`,`series_id`)\
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
}

MysqlPointRecord::~MysqlPointRecord() {
  /*
  if (_driver) {
    _driver->threadEnd();
  }
   */
}

#pragma mark - Public

void MysqlPointRecord::connect() throw(RtxException) {
  
  // split the tokenized string. we're expecting something like "HOST=tcp://localhost;USER=db_user;PWD=db_pass;DB=rtx_db_name"
  std::string tokenizedString = this->connectionString();
  if (RTX_STRINGS_ARE_EQUAL(tokenizedString, "")) {
    return;
  }
  
  // de-tokenize
  
  std::map<std::string, std::string> kvPairs;
  {
    boost::regex kvReg("([^=]+)=([^;]+);?"); // key - value pair
    boost::sregex_iterator it(tokenizedString.begin(), tokenizedString.end(), kvReg), end;
    for ( ; it != end; ++it) {
      kvPairs[(*it)[1]] = (*it)[2];
    }
    
    // if any of the keys are missing, just return.
    if (kvPairs.find("HOST") == kvPairs.end() ||
        kvPairs.find("UID") == kvPairs.end() ||
        kvPairs.find("PWD") == kvPairs.end() ||
        kvPairs.find("DB") == kvPairs.end() )
    {
      return;
    }
  }
  const std::string& host = kvPairs["HOST"];
  const std::string& user = kvPairs["UID"];
  const std::string& password = kvPairs["PWD"];
  const std::string& database = kvPairs["DB"];
  
  bool databaseDoesExist = false;
  try {
    _driver = get_driver_instance();
    _driver->threadInit();
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
      _connection->commit();
      cout << "Created new Database: " << database << endl;
    }
    
    _connection->setSchema(database);
    
    
    // build the queries, since preparedStatements can't specify table names.
    //string rangeSelect = "SELECT time, value FROM " + tableName + " WHERE series_id = ? AND time > ? AND time <= ?";
    string preamble = "SELECT time, value FROM points INNER JOIN timeseries_meta USING (series_id) WHERE name = ? AND ";
    string singleSelect = preamble + "time = ? order by time asc";
    string rangeSelect = preamble + "time >= ? AND time <= ? order by time asc";
    string nextSelect = preamble + "time > ? order by time asc LIMIT 1";
    string prevSelect = preamble + "time < ? order by time desc LIMIT 1";
    string singleInsert = "INSERT ignore INTO points (time, series_id, value) SELECT ?,series_id,? FROM timeseries_meta WHERE name = ?";
    
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
    _name = database;
  }
  catch (sql::SQLException &e) {
		handleException(e);
  }
}

bool MysqlPointRecord::isConnected() {
  if (!_connection || !checkConnection()) {
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
  this->checkConnection();
  try {
    boost::shared_ptr<sql::PreparedStatement> seriesNameStatement( _connection->prepareStatement("INSERT IGNORE INTO timeseries_meta (name) VALUES (?)") );
    seriesNameStatement->setString(1,recordName);
    seriesNameStatement->executeUpdate();
    _connection->commit();
  } catch (sql::SQLException &e) {
    handleException(e);
	}
  
  DB_PR_SUPER::registerAndGetIdentifier(recordName);
  
  return recordName;
}


PointRecord::time_pair_t MysqlPointRecord::range(const string& id) {
  Point last;
  Point first;
  
  if (checkConnection()) {
    
    _firstSelect->setString(1, id);
    boost::shared_ptr<sql::ResultSet> fResult( _firstSelect->executeQuery() );
    if( fResult && fResult->next() ) {
      time_t rowTime = fResult->getInt("time");
      double rowValue = fResult->getDouble("value");
      first = Point(rowTime, rowValue);
    }
    
    _lastSelect->setString(1, id);
    boost::shared_ptr<sql::ResultSet> lResult( _lastSelect->executeQuery() );
    if( lResult && lResult->next() ) {
      time_t rowTime = lResult->getInt("time");
      double rowValue = lResult->getDouble("value");
      last = Point(rowTime, rowValue);
    }
  }
  return make_pair(first.time, last.time);
}



#pragma mark -

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
  
  if (checkConnection()) {
    _rangeSelect->setString(1, id);
    _rangeSelect->setInt(2, (int)start);
    _rangeSelect->setInt(3, (int)end);
    boost::shared_ptr<sql::ResultSet> result( _rangeSelect->executeQuery() );
    while (result->next()) {
      time_t time = result->getInt("time");
      double value = result->getDouble("value");
      Point point(time, value);
      points.push_back(point);
    }
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
    _connection->commit();
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
    _connection->commit();
  }
}

void MysqlPointRecord::insertSingleNoCommit(const std::string& id, Point point) {
  _singleInsert->setInt(1, (int)point.time);
  // todo -- check this: _singleInsert->setUInt64(1, (uint64_t)time);
  _singleInsert->setDouble(2, point.value);
  _singleInsert->setString(3, id);
  int affected = 0;
  try {
    affected = _singleInsert->executeUpdate();
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
      removePointsStmt.reset( _connection->createStatement() );
      removePointsStmt->executeUpdate(removePoints);
      _connection->commit();
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
    
    truncatePointsStmt.reset( _connection->createStatement() );
    truncateKeysStmt.reset( _connection->createStatement() );
    
    truncatePointsStmt->executeUpdate(truncatePoints);
    truncateKeysStmt->executeUpdate(truncateKeys);
    
    _connection->commit();
  }
  catch (sql::SQLException &e) {
    handleException(e);
  }
}




#pragma mark - Private

Point MysqlPointRecord::selectSingle(const string& id, time_t time, boost::shared_ptr<sql::PreparedStatement> statement) {
  //cout << "hit MySql: " << time << endl;
  Point point;
  statement->setString(1, id);
  statement->setInt(2, (int)time);
  boost::shared_ptr<sql::ResultSet> result( statement->executeQuery() );
  if( result->next() ) {
    double rowValue = result->getDouble("value");
    time_t rowTime = result->getInt("time");
    point = Point(rowTime, rowValue);
  }
  return point;
}

bool MysqlPointRecord::checkConnection() {
  
  if(!_connection || _connection->isClosed()) {
    cerr << "mysql connection was closed. attempting to reconnect." << endl;
    this->connect();
  }
  return !(_connection->isClosed());
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




