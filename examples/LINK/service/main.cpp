#include <iostream>
#include <atomic>
#include <thread>
#include "LinkService.hpp"



using namespace std;

atomic<bool> _doRun;
void _signalHandler(int);
void _signalHandler(int signum) {
  cout << "Interrupt signal (" << signum << ") received.\n";
  _doRun = false;
}

int main(int argc, const char * argv[]) {
  
  RTX::LinkService svc(web::uri("http://127.0.0.1:3131"));
  svc.open().wait();
  
  _doRun = true;
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = _signalHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);
  
  while(_doRun) {
    this_thread::sleep_for(chrono::seconds(1));
  }
  
  cout << "SHUTTING DOWN SERVICE" << endl << flush;
  svc.close().wait();
}
