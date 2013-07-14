#ifndef LUA_TYPES
#define LUA_TYPES

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

typedef y::int32 ylib_int;

// Generic type.
template<typename T>
struct LuaType {
  // For generic types, this can't be defined automatically here. Should be
  // defined by a ylib_typedef macro in lua_api.h.
  static const y::string type_name;

  T& get(lua_State* state, ylib_int index) const;
  bool is(lua_State* state, ylib_int index) const;
  void push(lua_State* state, const T& arg) const;
};

// World scalar type.
template<>
struct LuaType<y::world> {
  static const y::string type_name;

  y::world get(lua_State* state, ylib_int index) const;
  bool is(lua_State* state, ylib_int index) const;
  void push(lua_State* state, y::world arg) const;
};

// Integer type.
template<>
struct LuaType<y::int32> {
  static const y::string type_name;

  y::int32 get(lua_State* state, ylib_int index) const;
  bool is(lua_State* state, ylib_int index) const;
  void push(lua_State* state, y::int32 arg) const;
};

// Boolean type.
template<>
struct LuaType<bool> {
  static const y::string type_name;

  bool get(lua_State* state, ylib_int index) const;
  bool is(lua_State* state, ylib_int index) const;
  void push(lua_State* state, bool arg) const;
};

// String type.
template<>
struct LuaType<y::string> {
  static const y::string type_name;

  y::string get(lua_State* state, ylib_int index) const;
  bool is(lua_State* state, ylib_int index) const;
  void push(lua_State* state, const y::string& arg) const;
};

// Array type.
template<typename T>
struct LuaType<y::vector<T>> {
  static const y::string type_name;

  y::vector<T> get(lua_State* state, ylib_int index) const;
  bool is(lua_State* state, ylib_int index) const;
  void push(lua_State* state, const y::vector<T>& arg) const;
};

// Standard type names.
const y::string LuaType<y::world>::type_name = "world";
const y::string LuaType<y::int32>::type_name = "int";
const y::string LuaType<bool>::type_name = "bool";
const y::string LuaType<y::string>::type_name = "string";

template<typename T>
const y::string LuaType<y::vector<T>>::type_name =
    "vector<" + LuaType<T>::type_name + ">";

// Generic type implementation.
template<typename T>
T& LuaType<T>::get(lua_State* state, ylib_int index) const
{
  void* v = lua_touserdata(state, index);
  T* t = reinterpret_cast<T*>(v);
  return *t;
}

template<typename T>
bool LuaType<T>::is(lua_State* state, ylib_int index) const
{
  return luaL_testudata(state, index, type_name.c_str());
}

template<typename T>
void LuaType<T>::push(lua_State* state, const T& arg) const
{
  T* t = reinterpret_cast<T*>(
      lua_newuserdata(state, sizeof(T)));
  luaL_getmetatable(state, type_name.c_str());
  lua_setmetatable(state, -2);
  new (t) T(arg);
}

// World scalar implementation.
y::world LuaType<y::world>::get(lua_State* state, ylib_int index) const
{
  return lua_tonumber(state, index);
}

bool LuaType<y::world>::is(lua_State* state, ylib_int index) const
{
  return lua_isnumber(state, index);
}

void LuaType<y::world>::push(lua_State* state, y::world arg) const
{
  lua_pushnumber(state, arg);
}

// Integer implementation.
y::int32 LuaType<y::int32>::get(lua_State* state, ylib_int index) const
{
  return lua_tointeger(state, index);
}

bool LuaType<y::int32>::is(lua_State* state, ylib_int index) const
{
  return lua_isnumber(state, index);
}

void LuaType<y::int32>::push(lua_State* state, y::int32 arg) const
{
  lua_pushinteger(state, arg);
}

// Boolean implementation.
bool LuaType<bool>::get(lua_State* state, ylib_int index) const
{
  return lua_toboolean(state, index);
}

bool LuaType<bool>::is(lua_State* state, ylib_int index) const
{
   return lua_isboolean(state, index);
}

void LuaType<bool>::push(lua_State* state, bool arg) const
{
  lua_pushboolean(state, arg);
}

// String implementation.
y::string LuaType<y::string>::get(lua_State* state, ylib_int index) const
{
  return y::string(lua_tostring(state, index));
}

bool LuaType<y::string>::is(lua_State* state, ylib_int index) const
{
  return lua_isstring(state, index);
}

void LuaType<y::string>::push(lua_State* state, const y::string& arg) const
{
  lua_pushstring(state, arg.c_str());
}

// Array implementation.
template<typename T>
y::vector<T> LuaType<y::vector<T>>::get(lua_State* state, ylib_int index) const
{
  y::vector<T> t;
  LuaType<T> element_type;

  ylib_int size = luaL_len(state, index);
  for (ylib_int i = 1; i <= size; ++i) {
    lua_rawgeti(state, index, i);
    t.emplace_back(element_type.get(state, lua_gettop(state)));
  }
  lua_pop(state, size);
  return t;
}

template<typename T>
bool LuaType<y::vector<T>>::is(lua_State* state, ylib_int index) const
{
  if (!lua_istable(state, index)) {
    return false;
  }
  LuaType<T> element_type;

  bool b = true;
  ylib_int size = luaL_len(state, index);
  for (ylib_int i = 1; i <= size; ++i) {
    lua_rawgeti(state, index, i);
    b &= element_type.is(state, lua_gettop(state));
  }
  lua_pop(state, size);
  return b;
}

template<typename T>
void LuaType<y::vector<T>>::push(
    lua_State* state, const y::vector<T>& arg) const
{
  lua_createtable(state, arg.size(), 0);
  LuaType<T> element_type;

  ylib_int index = lua_gettop(state);
  for (y::size i = 0; i < arg.size(); ++i) {
    element_type.push(state, arg[i]);
    lua_rawseti(state, index, 1 + i);
  }
}

#endif
