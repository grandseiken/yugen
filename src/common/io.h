#ifndef COMMON__IO_H
#define COMMON__IO_H

#include <iostream>
#include <iomanip>
#include "string.h"

namespace y {

  typedef std::ostream ostream;
  typedef std::istream istream;

  static auto& cout = std::cout;
  static auto& cerr = std::cerr;
  static auto& cin = std::cin;

  using std::endl;
  using std::setfill;
  using std::setw;
  using std::setprecision;
  using std::fixed;

}

#endif
