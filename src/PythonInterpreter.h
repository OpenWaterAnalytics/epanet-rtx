//
//  PythonInterpreter.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 8/27/14.
//
//

#ifndef __epanet_rtx__PythonInterpreter__
#define __epanet_rtx__PythonInterpreter__


#include <iostream>

#include "rtxMacros.h"

//! singleton python interpreter object

namespace RTX {
  class PythonInterpreter {
  public:
    RTX_SHARED_POINTER(PythonInterpreter);
    static PythonInterpreter::sharedPointer sharedInterpreter() {
      static PythonInterpreter::sharedPointer instance( new PythonInterpreter );
      return instance;
    };
    
    void exec(std::string execStr);
    int identifier;
    
  private:
    PythonInterpreter();
    PythonInterpreter(PythonInterpreter const&); // don't implement
    void operator=(PythonInterpreter const&); // don't implement
  };
}




#endif /* defined(__epanet_rtx__PythonInterpreter__) */
