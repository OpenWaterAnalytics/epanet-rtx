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
    
    void dbConnect() throw(RtxException);
    
    bool isConnected() {return _connected;};
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    const std::map<std::string,Units> identifiersAndUnits();
    void truncate();

    std::string connectionString();
    void setConnectionString(const std::string& str);
    
    std::string host, user, pass, db;
    int port;
    
    bool supportsSinglyBoundedQueries() {return true;};
    bool shouldSearchIteratively() {return true;};
    bool supportsUnitsColumn() {return false;};
    
  protected:
    std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    void removeRecord(const std::string& id);
    
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
    
    static MetricInfo metricInfoFromName(const std::string& name);
    static const std::string nameFromMetricInfo(MetricInfo info);
    static std::string properId(const std::string& id);
    std::string _influxIdForTsId(const std::string& id);
    DbPointRecord::Query queryPartsFromMetricId(const std::string& name);
    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
