//
//  PythonInterpreter.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 8/27/14.
//
//

#include "PythonInterpreter.h"


#include <boost/python.hpp>


using namespace RTX;
using namespace std;
using namespace boost::python;

boost::python::object main_module, main_namespace;


PythonInterpreter::PythonInterpreter() {
  // private constructor
  int good = 0;
  
  try {
    Py_Initialize();
    good = Py_IsInitialized();
    //main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
    main_module = import("__main__");
    main_namespace = main_module.attr("__dict__");
  }
  catch (error_already_set) {
    PyErr_Print();
  }
  
}

void PythonInterpreter::exec(std::string execStr) {
  
  try {
    cout << "EXECUTING PYTHON: " << execStr << endl;
    boost::python::exec((boost::python::str) execStr, main_namespace);
  }
  catch (error_already_set) {
    cerr << "python runtime error :: " << execStr << endl;
    PyErr_Print();
  }
  
}

double PythonInterpreter::doubleValueFromEval(std::string evalStr) {
  try {
    object valObj = eval((boost::python::str)evalStr, main_namespace);
    double val = extract<double>(valObj);
    return val;
  }
  catch (error_already_set) {
    cerr << "python runtime error :: extract double" << endl;
    PyErr_Print();
    return 0;
  }
}

long int PythonInterpreter::longIntValueFromEval(std::string evalStr) {
  try {
    object valObj = eval((boost::python::str)evalStr, main_namespace);
    long int val = extract<long>(valObj);
    return val;
  }
  catch (error_already_set) {
    cerr << "python runtime error :: extract long" << endl;
    PyErr_Print();
    return 0;
  }
}

int PythonInterpreter::intValueFromEval(std::string evalStr) {
  try {
    object valObj = eval((boost::python::str)evalStr, main_namespace);
    int val = extract<int>(valObj);
    return val;
  }
  catch (error_already_set) {
    cerr << "python runtime error :: extract long" << endl;
    PyErr_Print();
    return 0;
  }
}

