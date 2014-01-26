#ifndef COMMON_PAIR_H
#define COMMON_PAIR_H

#include "math.h"

#include <utility>
#include <boost/functional/hash.hpp>

namespace y {

  template<typename T, typename U>
  using pair = std::pair<T, U>;
  using std::make_pair;

}

namespace std {

  template<typename T, typename U>
  struct hash<y::pair<T, U>> {
    y::size operator()(const y::pair<T, U>& arg) const
    {
      std::hash<T> t;
      std::hash<U> u;

      y::size seed = 0;
      boost::hash_combine(seed, t(arg.first));
      boost::hash_combine(seed, u(arg.second));
      return seed;
    }
  };

}

#endif
