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
   \fn virtual void MysqlPointRecord::connect(const string& host, const string& user, const string& password, const string& database);
   \brief Get a Point with a specific name at a specific time.
   \param host The name, IP, or socket address of the MySQL database.
   \param user The user name for the database.
   \param password The password for accessing the database.
   \param database The name of the Database you want to use.
   \sa PointRecord
   
   The MySQL connector is based on the JDBC-API, so use the format "tcp://ipaddress.or.name.of.server" or "unix://path/to/unix_socket_file".
   If the Database name passed in does not exist, then it is created for you.
   
   */
  
  class MysqlPointRecord : public DbPointRecord {
  public:
    RTX_SHARED_POINTER(MysqlPointRecord);
    MysqlPointRecord();
    virtual ~MysqlPointRecord();
    
    virtual void connect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName);
    virtual std::vector<std::string> identifiers();
    virtual bool isPointAvailable(const string& identifier, time_t time);
    virtual Point point(const string& identifier, time_t time);
    virtual Point pointBefore(const string& identifier, time_t time);
    virtual Point pointAfter(const string& identifier, time_t time);
    virtual void addPoint(const string& identifier, Point point);
    virtual void addPoints(const string& identifier, std::vector<Point> points);
    virtual void reset();
    virtual void reset(const string& identifier);
    virtual Point firstPoint(const string& id);
    virtual Point lastPoint(const string& id);
    
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    bool _connectionOk;
    void insertSingle(const string& identifier, time_t time, double value);
    Point selectSingle(const string& identifier, time_t time, boost::shared_ptr<sql::PreparedStatement> statement);
    std::vector<Point>selectRange(const string& identifier, time_t start, time_t end);
    void handleException(sql::SQLException &e);
    string _name;
    sql::Driver* _driver;
    boost::shared_ptr<sql::Connection> _connection;
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
