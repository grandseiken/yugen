#ifndef LUA_TYPES
#define LUA_TYPES

#include "common.h"
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

typedef y::int32 ylib_int;

// Can hold a variety of different Lua types.
// TODO: make this work with more types. Userdata types are tricky, because we
// need to be able to determine arbitrary types from Lua userdatums.
struct LuaValue {
  LuaValue(y::world world);
  LuaValue(bool boolean);
  LuaValue(const y::string& string);
  LuaValue(const y::vector<LuaValue>& array);

  enum {
    WORLD,
    BOOLEAN,
    STRING,
    ARRAY,
  } type;

  y::world world;
  bool boolean;
  y::string string;
  y::vector<LuaValue> array;
};

// Generic Lua-allocated and copied type.
template<typename T>
struct LuaType {
  // For generic types, this can't be defined automatically here. Should be
  // defined by a ylib_*typedef macro in lua_api.h.
  static const y::string type_name;

  inline T& get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, const T& arg) const;
};

// World scalar type.
template<>
struct LuaType<y::world> {
  static const y::string type_name;

  inline y::world get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, y::world arg) const;
};

// Integer type.
template<>
struct LuaType<y::int32> {
  static const y::string type_name;

  inline y::int32 get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, y::int32 arg) const;
};

// Boolean type.
template<>
struct LuaType<bool> {
  static const y::string type_name;

  inline bool get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, bool arg) const;
};

// String type.
template<>
struct LuaType<y::string> {
  static const y::string type_name;

  inline y::string get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, const y::string& arg) const;
};

// Array type.
template<typename T>
struct LuaType<y::vector<T>> {
  static const y::string type_name;

  inline y::vector<T> get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, const y::vector<T>& arg) const;
};

// Generic LuaValue union type.
template<>
struct LuaType<LuaValue> {
  static const y::string type_name;

  inline LuaValue get(lua_State* state, ylib_int index) const;
  inline bool is(lua_State* state, ylib_int index) const;
  inline void push(lua_State* state, const LuaValue& arg) const;
};

// Standard type names.
template<typename T>
const y::string LuaType<y::vector<T>>::type_name =
    "array<" + LuaType<T>::type_name + ">";

// Generic Lua-allocated and copied implementation.
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

// Generic LuaValue union implementation.
LuaValue LuaType<LuaValue>::get(lua_State* state, ylib_int index) const
{
  LuaType<y::world> world;
  if (world.is(state, index)) {
    return LuaValue(world.get(state, index));
  }

  LuaType<bool> boolean;
  if (boolean.is(state, index)) {
    return LuaValue(boolean.get(state, index));
  }

  LuaType<y::string> string;
  if (string.is(state, index)) {
    return LuaValue(string.get(state, index));
  }

  LuaType<y::vector<LuaValue>> array;
  if (array.is(state, index)) {
    return LuaValue(array.get(state, index));
  }

  return LuaValue(0.);
}

bool LuaType<LuaValue>::is(lua_State* state, ylib_int index) const
{
  LuaType<y::world> world;
  LuaType<bool> boolean;
  LuaType<y::string> string;
  LuaType<y::vector<LuaValue>> array;

  return
      world.is(state, index) ||
      boolean.is(state, index) ||
      string.is(state, index) ||
      array.is(state, index);
}

void LuaType<LuaValue>::push(lua_State* state, const LuaValue& arg) const
{
  switch (arg.type) {
    case LuaValue::WORLD:
      LuaType<y::world>().push(state, arg.world);
      break;
    case LuaValue::BOOLEAN:
      LuaType<bool>().push(state, arg.boolean);
      break;
    case LuaValue::STRING:
      LuaType<y::string>().push(state, arg.string);
      break;
    case LuaValue::ARRAY:
      LuaType<y::vector<LuaValue>>().push(state, arg.array);
      break;
    default: {}
  }
}

#endif
