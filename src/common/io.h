#ifndef COMMON__IO_H
#define COMMON__IO_H

#include <iostream>
#include <iomanip>
#include "string.h"

namespace y {

  typedef std::ostream ostream;
  typedef std::istream istream;

  static decltype(std::cout)& cout = std::cout;
  static decltype(std::cerr)& cerr = std::cerr;
  static decltype(std::cin)& cin = std::cin;

  using std::endl;
  using std::setfill;
  using std::setw;
  using std::setprecision;
  using std::fixed;

}

#endif
