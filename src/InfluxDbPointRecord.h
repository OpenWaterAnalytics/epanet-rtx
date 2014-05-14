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
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);
    virtual std::vector<std::string> identifiers();
    //virtual std::vector<std::pair<std::string, Units> >availableData();
    virtual void truncate() {}; // specific implementation must override this

    
    virtual time_pair_t range(const string& id);
    
    std::string host, user, pass, db;
    int port;
    
    virtual bool supportsBoundedQueries() {return false;};
    
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
    
    
    JsonDocPtr documentFromUrl(const std::string& url);
    JsonDocPtr insertionDocumentFromPoints(const std::string& tsName, std::vector<Point> points);
    const std::string serializedDocument(JsonDocPtr doc);
    std::vector<Point> pointsFromDoc(JsonDocPtr doc);
    const std::string urlForQuery(const std::string& query, bool appendTimePrecision = true); // unencoded query
    const std::string urlEncode(std::string s);
    void postPointsWithBody(const std::string& body);
    
//    boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;
    bool _connected;
    
    PointRecord::time_pair_t _range;
    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
