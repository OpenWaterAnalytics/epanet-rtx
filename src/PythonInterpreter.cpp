//
//  PythonInterpreter.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 8/27/14.
//
//

#include "PythonInterpreter.h"

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>


using namespace RTX;
using namespace std;
using namespace boost::python;

PythonInterpreter::PythonInterpreter() {
  // private constructor
  
  Py_Initialize();
  
  
}

void PythonInterpreter::exec(std::string execStr) {
  
  cout << "EXECUTING PYTHON: " << execStr << endl;
  
}