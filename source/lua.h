#ifndef LUA_H
#define LUA_H

#include "common.h"
#include "vector.h"

class Filesystem;
class GameStage;
struct lua_State;

class Script : public y::no_copy {
public:

  Script(GameStage& stage, const y::string& path, const y::string& contents,
         const y::ivec2& origin, const y::ivec2& region);
  ~Script();

  const y::string& get_path() const;

  // A Script's region is initially provided on load from the CellMap (or when
  // dynamically created). The origin is always centered in the region. Scripts
  // are created when their region overlaps the WorldWindow, and automatically
  // destroyed when their current region is completely outside it.
  const y::ivec2& get_origin() const;
  const y::ivec2& get_region() const;
  void set_origin(const y::ivec2& origin);
  void set_region(const y::ivec2& region);

  bool has_function(const y::string& function_name) const;
  void call(const y::string& function_name);

private:

  y::string _path;
  lua_State* _state;

  y::ivec2 _origin;
  y::ivec2 _region;

};

#endif
