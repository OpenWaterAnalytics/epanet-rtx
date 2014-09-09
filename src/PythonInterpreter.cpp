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

boost::python::object main_module, main_namespace;


PythonInterpreter::PythonInterpreter() {
  // private constructor
  
  Py_Initialize();
  main_module = import("__main__");
  main_namespace = main_module.attr("__dict__");
  
}

void PythonInterpreter::exec(std::string execStr) {
  
  cout << "EXECUTING PYTHON: " << execStr << endl;
  
}

double PythonInterpreter::doubleValueFromEval(std::string evalStr) {
  object valObj = eval((boost::python::str)evalStr, main_namespace);
  double val = extract<double>(valObj);
  return val;
}

long int PythonInterpreter::longIntValueFromEval(std::string evalStr) {
  object valObj = eval((boost::python::str)evalStr, main_namespace);
  long int val = extract<long>(valObj);
  return val;
}

int PythonInterpreter::intValueFromEval(std::string evalStr) {
  object valObj = eval((boost::python::str)evalStr, main_namespace);
  int val = extract<int>(valObj);
  return val;
}

