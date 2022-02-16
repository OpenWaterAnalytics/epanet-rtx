//
//  SendPointsCoroutine.cpp
//  epanet-rtx static deps
//
//  Created by Devine, Cameron - Xylem on 2/15/22.
//
#include "SendPointsCoroutine.hpp"

using namespace oatpp;
using namespace nlohmann;

async::Action SendPointsCoroutine::act() {
  return m_client->sendPoints(_authString, _contentEncoding, _db, _precision, _data).callbackTo(&SendPointsCoroutine::onResponse);
}

async::Action SendPointsCoroutine::onResponse(const std::shared_ptr<Response>& response){
  int code = response->getStatusCode();
  oatpp::String desc = response->getStatusDescription();
  
  switch(code) {
    case 204:
    case 200:
      break;
    default:
      OATPP_LOGD("SENDPONTSASYNC", "send points to influx: POST returned %i - %s", code, desc->c_str());
      return response->readBodyToStringAsync().callbackTo(&SendPointsCoroutine::onBody);
    return finish();
  }
    
  return response->readBodyToStringAsync().callbackTo(&SendPointsCoroutine::onBody);
}

async::Action SendPointsCoroutine::onBody(const oatpp::String& body){
  std::string bodyStr = body.getValue("");
  json jsBody = json::parse(bodyStr);
  OATPP_LOGD("SENDPONTSASYNC", "%s", jsBody.dump().c_str());
  return finish();
}
