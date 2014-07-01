#ifndef LOG_H
#define LOG_H

#include <iostream>

namespace {

inline void logg(std::ostream&)
{
}

template<typename T, typename... U>
inline void logg(std::ostream& o, const T& t, const U&... args)
{
  o << t;
  logg(o, args...);
}

inline void logv() {}

template<typename T, typename... U>
inline void logv(const T&, const U&... args)
{
  logv(args...);
}

template<typename... T>
inline void loge(std::ostream& o, const T&... args)
{
  logg(o, args...);
  o << std::endl;
}

template<typename... T>
inline void log_debug(const T&... args)
{
#ifdef DEBUG
  loge(y::cout, args...);
#else
  logv(args...);
#endif
}

template<typename... T>
inline void logg_debug(const T&... args)
{
#ifdef DEBUG
  logg(y::cout, args...);
#else
  logv(args...);
#endif
}

template<typename... T>
inline void log_info(const T&... args)
{
  loge(std::cout, args...);
}

template<typename... T>
inline void logg_info(const T&... args)
{
  logg(std::cout, args...);
}

template<typename... T>
inline void log_err(const T&... args)
{
  loge(std::cerr, args...);
}

template<typename... T>
inline void logg_err(const T&... args)
{
  logg(std::cerr, args...);
}

// End anonymous namespace.
}

#endif
