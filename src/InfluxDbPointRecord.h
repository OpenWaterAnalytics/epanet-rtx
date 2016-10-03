#ifndef __epanet_rtx__InfluxDbPointRecord__
#define __epanet_rtx__InfluxDbPointRecord__

#include <boost/asio.hpp>
#include <boost/signals2/mutex.hpp>

#include <iostream>

#include "DbPointRecord.h"

namespace RTX {
  class ITaskWrapper {
    // empty implementation for task subs
  };
  
  class InfluxDbPointRecord : public DbPointRecord {
    
  public:
    
    RTX_BASE_PROPS(InfluxDbPointRecord);
    
    InfluxDbPointRecord();
    
    void dbConnect() throw(RtxException);
    
    bool isConnected() {return _connected;};
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    IdentifierUnitsList identifiersAndUnits();
    void truncate();

    std::string connectionString();
    void setConnectionString(const std::string& str);
    
    std::string host, user, pass, db;
    int port;
    
    bool supportsSinglyBoundedQueries() {return true;};
    bool shouldSearchIteratively() {return false;};
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
    
    const std::string insertionLineFromPoints(const std::string& tsName, std::vector<Point> points);
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
    
    std::shared_ptr<boost::signals2::mutex> _mutex;
    std::shared_ptr<ITaskWrapper> _sendTask;
    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
