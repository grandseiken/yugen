#ifndef COMMON__STRING_H
#define COMMON__STRING_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace y {

  typedef std::string string;
  typedef std::stringstream sstream;
  typedef std::ostream ostream;
  typedef std::istream istream;

  static decltype(std::cout)& cout = std::cout;
  static decltype(std::cerr)& cerr = std::cerr;
  static decltype(std::cin)& cin = std::cin;

  using std::stoi;
  using std::to_string;
  using std::endl;

  using std::setfill;
  using std::setw;
  using std::setprecision;
  using std::fixed;

}

#endif
