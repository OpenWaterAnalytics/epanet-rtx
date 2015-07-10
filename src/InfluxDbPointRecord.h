#ifndef __epanet_rtx__InfluxDbPointRecord__
#define __epanet_rtx__InfluxDbPointRecord__

#include <iostream>

#include "DbPointRecord.h"
#include <rapidjson/document.h>

#include <boost/asio.hpp>



namespace RTX {
  typedef boost::shared_ptr<rapidjson::Document> JsonDocPtr;

  class InfluxDbPointRecord : public DbPointRecord {
    
  public:
    RTX_SHARED_POINTER(InfluxDbPointRecord);
    
    InfluxDbPointRecord();
    
    virtual void dbConnect() throw(RtxException);
    
    virtual bool isConnected() {return _connected;};
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    virtual std::vector< nameUnitsPair > identifiersAndUnits();
    
    
    void truncate();

    std::string connectionString();
    void setConnectionString(const std::string& str);
    
    std::string host, user, pass, db;
    int port;
    
    virtual bool supportsBoundedQueries() {return false;};
    bool supportsUnitsColumn() {return false;};
    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    
  private:
    
    typedef struct {
      boost::asio::ip::tcp::iostream sockStream;
      std::string httpVersion, statusMessage;
      unsigned int statusCode;
    } InfluxConnectInfo_t;
    
//    InfluxConnectInfo_t connectionInfo;
    
    
    JsonDocPtr jsonFromPath(const std::string& url);
    const std::string insertionDataFromPoints(const std::string& tsName, std::vector<Point> points);
    
//    JsonDocPtr insertionJsonFromPoints(const std::string& tsName, std::vector<Point> points);
//    const std::string serializedJson(JsonDocPtr doc);
    std::vector<Point> pointsFromJson(JsonDocPtr doc);
    const std::string urlForQuery(const std::string& query, bool appendTimePrecision = true); // unencoded query
    const std::string urlEncode(std::string s);
//    void postPointsWithBody(const std::string& body);
    
    const std::string insertionStringWithPoints(const std::string& tsName, std::vector<Point> points);
    void sendPointsWithString(const std::string& content);
    
//    boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;
    bool _connected;
    
    PointRecord::time_pair_t _range;
    
    class MetricInfo {
    public:
      std::map<std::string, std::string> tags;
      std::string measurement;
    };
    
    MetricInfo metricInfoFromName(const std::string& name);
    const std::string nameFromMetricInfo(MetricInfo info);
    const std::string nameAndWhereClause(const std::string& name);
    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
