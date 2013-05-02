#ifndef LUA_H
#define LUA_H

#include "common.h"

class Filesystem;
struct lua_State;

class Script {
public:

  Script(const y::string& path, const y::string& contents);
  ~Script();

  const y::string& get_path() const;

  void call(const y::string& function_name);

private:

  y::string _path;
  lua_State* _state;

};

#endif
