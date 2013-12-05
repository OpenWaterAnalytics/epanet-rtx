//
//  MysqlPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_MysqlPointRecord_h
#define epanet_rtx_MysqlPointRecord_h

#include "DbPointRecord.h"
#include <stdexcept>
#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>

namespace RTX {
  
  /*! \class MysqlPointRecord
   \brief A persistence layer for MySQL databases
   
   Uses the MySQL C++ library for point storage and retrieval. 
   Primarily used for persistent caching of TimeSeries streams after intensive filtering that you only want to do once in a lifetime
   such as for saving simulation results or cleaned SCADA data.
   
   */
  
  /*!
   
   The MySQL connector is based on the JDBC-API, so use the format "tcp://ipaddress.or.name.of.server" or "unix://path/to/unix_socket_file".
   If the Database name passed in does not exist, then it is created for you.
   
   */
  
  using std::string;
  
  class MysqlPointRecord : public DbPointRecord {
  public:
    
    class mysql_connection_t {
    public:
      string host,uid,pwd,db;
    };
    
    RTX_SHARED_POINTER(MysqlPointRecord);
    MysqlPointRecord();
    virtual ~MysqlPointRecord();
    
    string host();
    void setHost(string host);
    
    string uid();
    void setUid(string uid);
    
    string pwd();
    void setPwd(string pwd);
    
    string db();
    void setDb(string db);
    
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);
    virtual std::vector<std::string> identifiers();
    virtual std::vector<std::pair<std::string, Units> >availableData();
    
    virtual time_pair_t range(const string& id);
    virtual std::ostream& toStream(std::ostream &stream);
    
    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    virtual void truncate();
    
  private:
    bool _connected;
    mysql_connection_t _connectionInfo;
    void insertSingleNoCommit(const std::string& id, Point point);
    bool checkConnection();
    void insertSingle(const string& id, time_t time, double value);
    Point selectSingle(const string& id, time_t time, boost::shared_ptr<sql::PreparedStatement> statement);
    std::vector<Point> pointsFromResultSet(boost::shared_ptr<sql::ResultSet> result);
    
    void handleException(sql::SQLException &e);
    string _name;
    sql::Driver* _driver;
    boost::shared_ptr<sql::Connection> _mysqlCon;
    // prepared statements for selecting, inserting
    boost::shared_ptr<sql::PreparedStatement>  _rangeSelect,
                                               _singleSelect,
                                               _nextSelect,
                                               _previousSelect,
                                               _singleInsert,
                                               _firstSelect,
                                               _lastSelect;
    
  };

  
}

#endif
