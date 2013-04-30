#include "lua.h"
#include "filesystem.h"

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

// Test function.
int hello(lua_State* state)
{
  // Example error-checking.
  if (lua_gettop(state) == 0 || !lua_isnumber(state, -1)) {
    std::cerr << "hello called with invalid thing" << std::endl;
  }
  std::cout << "hello you passed " << lua_tointeger(state, 1) << std::endl;

  // Return number of return values.
  return 0;
}

int twice(lua_State* state)
{
  lua_pushinteger(state, 2 * lua_tointeger(state, 1));

  // Return number of return values.
  return 1;
}

// TODO: figure out code-sharing between scripts; engine integration,
// library, etc.
Script::Script(const Filesystem& filesystem, const y::string& path)
  : _path(path)
  // Use standard allocator and panic function.
  , _state(luaL_newstate())
{
  filesystem.read_file(_data, path);
  // Register test functions.
  lua_register(_state, "hello", hello);
  lua_register(_state, "twice", twice);
}

Script::~Script()
{
  lua_close(_state);
}

const y::string& Script::get_path() const
{
  return _path;
}

void Script::run() const
{
  // Chunk reader function.
  struct local {
    const y::string& data;
    bool has_read;

    static const char* read(lua_State* state, void* data, y::size* size)
    {
      (void)state;
      local* data_struct = reinterpret_cast<local*>(data);
      if (data_struct->has_read) {
        *size = 0;
        return y::null;
      }
      data_struct->has_read = true;
      *size = data_struct->data.length();
      return data_struct->data.c_str();
    }
  };

  local data_struct{_data, false};
  if (lua_load(_state, local::read, &data_struct, _path.c_str(), y::null) ||
      // TODO: use a nice error function with tracebacks.
      lua_pcall(_state, 0, 0, 0)) {
    const char* error = lua_tostring(_state, -1);
    if (error) {
      std::cerr << "Running script " << _path << " failed: " <<
          error << std::endl;
    }
    else {
      std::cerr << "Running script " << _path << " failed" << std::endl;
    }
  }
}
