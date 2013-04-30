#ifndef LUA_H
#define LUA_H

#include "common.h"

class Filesystem;
struct lua_State;

class Script {
public:

  Script(const Filesystem& filesystem, const y::string& path);
  ~Script();

  const y::string& get_path() const;

  void run() const;

private:

  y::string _path;
  y::string _data;
  lua_State* _state;

};

#endif
