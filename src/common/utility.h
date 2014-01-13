#ifndef COMMON__UTILITY_H
#define COMMON__UTILITY_H

#include <boost/utility.hpp>
#include <type_traits>

namespace y {

  typedef boost::noncopyable no_copy;

  template<bool B, typename T>
  using enable_if = std::enable_if<B, T>;
  template<typename T, typename U>
  using is_same = std::is_same<T, U>;

  using std::remove_cv;
  using std::remove_const;
  using std::remove_volatile;

}

#endif