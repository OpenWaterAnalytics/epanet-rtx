#include <iostream>

#include "LinkService.hpp"


int main(int argc, const char * argv[]) {
  
  RTX::LinkService svc(web::uri("http://127.0.0.1:3131"));
  
  if (argc >= 6) {
    svc._metricDatabase.host = string(argv[1]);
    svc._metricDatabase.port = string(argv[2]);
    svc._metricDatabase.db   = string(argv[3]);
    svc._metricDatabase.user = string(argv[4]);
    svc._metricDatabase.pass = string(argv[5]);
  }
  
  else {
    svc._metricDatabase.host = "flux.citilogics.io";
    svc._metricDatabase.port = "8086";
    svc._metricDatabase.db   = "metrics_test_link_service";
    svc._metricDatabase.user = "metrics";
    svc._metricDatabase.pass = "metrics";
  }
  
  svc.open().wait();
  
  while (true) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "x") {
      break;
    }
  }
  
  svc.close().wait();
}
