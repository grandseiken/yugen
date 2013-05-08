#ifndef LUA_H
#define LUA_H

#include "common.h"

class Filesystem;
class GameStage;
struct lua_State;

class Script {
public:

  Script(GameStage& stage, const y::string& path, const y::string& contents);
  ~Script();

  const y::string& get_path() const;

  template<typename T>
  void set_registry_value(const y::string& key, const T& value);

  void call(const y::string& function_name);

private:

  y::string _path;
  lua_State* _state;

};

#endif
