#include "InfluxUdpPointRecord.h"
#include <boost/asio.hpp>

using namespace std;
using namespace RTX;
using boost::asio::ip::udp;

InfluxUdpPointRecord::InfluxUdpPointRecord() {
  _sendFuture = async(launch::async, [&](){return;});
  
  this->conn.port = 8089; // default udp for influx
}

void InfluxUdpPointRecord::dbConnect() throw(RTX::RtxException) {
  _connected = false;
  boost::asio::io_service io_service;
  try {
    
    boost::asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), this->conn.host, to_string(this->conn.port));
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket socket(io_service);
    socket.open(udp::v4());
    socket.close();
  } catch (const std::exception &err) {
    DebugLog << "could not connect to UDP endpoint" << EOL << flush;
    this->errorMessage = "Invalid UDP Endpoint";
    return;
  }
  this->errorMessage = "Connected";
  _connected = true;
}

// Line protocol requires unix-nano unles qualified by HTTP-GET fields,
// which of course we don't have over UDP
string InfluxUdpPointRecord::formatTimestamp(time_t t) {
  return to_string(t) + "000000000";
};


void InfluxUdpPointRecord::sendPointsWithString(const std::string &content) {
  if (_sendFuture.valid()) {
    _sendFuture.wait();
  }
  string body(content);
  _sendFuture = async(launch::async, [&,body]() {
    boost::asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), this->conn.host, to_string(this->conn.port));
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket socket(io_service);
    socket.open(udp::v4());
    boost::system::error_code err;
    socket.send_to(boost::asio::buffer(body, body.size()), receiver_endpoint, 0, err);
    if (err) {
      DebugLog << "UDP SEND ERROR: " << err.message() << EOL << flush;
    }
    socket.close();
  });
}

