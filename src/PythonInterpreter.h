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


namespace RTX {
  
  /*!
   \class PythonInterpreter
   \brief A singleton python interpreter supporting the EPANET-RTX library
   */
  
  /*!
   \fn static PythonInterpreter::_sp PythonInterpreter::sharedInterpreter()
   \brief Get the shared interpreter instance.
   \return a shared pointer to the instance.
   */
  /*!
   \fn void PythonInterpreter::exec(std::string execStr)
   \brief Execute a python command
   \param execStr the command to execute
   \return none
   */
  /*!
   \fn double PythonInterpreter::doubleValueFromEval(std::string evalStr)
   \brief Get a double from evaluating a python string.
   \param evalStr The python string to evaluate
   \return The evaluated value
  */
  
  
  
  class PythonInterpreter {
  public:
    RTX_SHARED_POINTER(PythonInterpreter);
    static PythonInterpreter::_sp sharedInterpreter() {
      static PythonInterpreter::_sp instance( new PythonInterpreter );
      return instance;
    };
    
    void exec(std::string execStr);
    double doubleValueFromEval(std::string evalStr);
    
    //! Get a Long Int from evaluating a python string
    long int longIntValueFromEval(std::string evalStr);
    
    //! Get an Int from evaluating a python string
    int intValueFromEval(std::string evalStr);
    
    
  private:
    PythonInterpreter();
    PythonInterpreter(PythonInterpreter const&); // don't implement
    void operator=(PythonInterpreter const&); // don't implement
  };
}




#endif /* defined(__epanet_rtx__PythonInterpreter__) */
