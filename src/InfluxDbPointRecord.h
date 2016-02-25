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
    
    virtual void beginBulkOperation();
    virtual void endBulkOperation();
    
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
    
    JsonDocPtr jsonFromPath(const std::string& url);
    const std::string insertionLineFromPoints(const std::string& tsName, std::vector<Point> points);
    
    std::vector<Point> pointsFromJson(JsonDocPtr doc);
    const std::string urlForQuery(const std::string& query, bool appendTimePrecision = true); // unencoded query
    const std::string urlEncode(std::string s);
    
    void sendPointsWithString(const std::string& content);
    bool _connected;
    
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
    
    void commitTransactionLines();
    std::vector<std::string> _transactionLines;
    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
