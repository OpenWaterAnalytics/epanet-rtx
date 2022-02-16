//
//  SendPointsCoroutine.h
//  epanet-rtx
//
//  Created by Devine, Cameron - Xylem on 2/15/22.
//

#ifndef SendPointsCoroutine_h
#define SendPointsCoroutine_h

#include "InfluxClient.hpp"

#include "oatpp/core/async/Executor.hpp"
#include "nlohmann/json.hpp"

class SendPointsCoroutine : public oatpp::async::Coroutine<SendPointsCoroutine>{
private:
  typedef oatpp::web::protocol::http::incoming::Response Response;
private:
  std::shared_ptr<InfluxClient> m_client;
  oatpp::String _authString, _contentEncoding, _db, _precision, _data;
public:
  SendPointsCoroutine(const std::shared_ptr<InfluxClient> client, const std::string authString,
                  const std::string contentEncoding, const std::string db, const std::string precision,
                  const std::string data )
          : m_client(client), _authString(authString)
          , _contentEncoding(contentEncoding), _db(db), _precision(precision), _data(data){}
  
  Action act() override;
  Action onResponse(const std::shared_ptr<Response>& response);
  Action onBody(const oatpp::String& body);
};
#endif /* SendPointsCoroutine_h */
