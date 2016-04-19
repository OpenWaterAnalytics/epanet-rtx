#include <iostream>

#include "LinkService.hpp"


int main(int argc, const char * argv[]) {
  
  
  RTX::LinkService svc(web::uri("http://127.0.0.1:3131"));
  
  svc.open().wait();
  
  std::string line;
  std::getline(std::cin, line);
  
  svc.close().wait();
}
