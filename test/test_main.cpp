#define __AS_MAIN
  #include "test_main.h"
#undef __AS_MAIN

#include <iostream>

boost::unit_test::test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[])
{
  std::cout << "using obsolete init" << std::endl;
  return 0;
}
