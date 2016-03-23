//
//  main.cpp
//  FLUX-service
//
//  Created by Sam Hatchett on 3/23/16.
//
//

#include <iostream>

#include "FluxService.hpp"


int main(int argc, const char * argv[]) {
  
  
  RTX::FluxService svc(web::uri("http://localhost:3131"));
  
  svc.open().wait();
  
  std::string line;
  std::getline(std::cin, line);
  
  svc.close().wait();
}
