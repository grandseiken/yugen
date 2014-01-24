#ifndef LOG_H
#define LOG_H

#include "common/io.h"

namespace {

inline void logg(y::ostream&)
{
}

template<typename T, typename... U>
inline void logg(y::ostream& o, T t, U... args)
{
  o << t;
  logg(o, args...);
}

inline void logv() {}

template<typename T, typename... U>
inline void logv(T, U... args)
{
  logv(args...);
}

template<typename... T>
inline void loge(y::ostream& o, T... args)
{
  logg(o, args...);
  o << y::endl;
}

template<typename... T>
inline void log_debug(T... args)
{
#ifdef DEBUG
  loge(y::cout, args...);
#else
  logv(args...);
#endif
}

template<typename... T>
inline void logg_debug(T... args)
{
#ifdef DEBUG
  logg(y::cout, args...);
#else
  logv(args...);
#endif
}

template<typename... T>
inline void log_info(T... args)
{
  loge(y::cout, args...);
}

template<typename... T>
inline void logg_info(T... args)
{
  logg(y::cout, args...);
}

template<typename... T>
inline void log_err(T... args)
{
  loge(y::cerr, args...);
}

template<typename... T>
inline void logg_err(T... args)
{
  logg(y::cerr, args...);
}

// End anonymous namespace.
}

#endif
