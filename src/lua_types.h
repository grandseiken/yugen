#ifndef LUA_TYPES_H
#define LUA_TYPES_H

#include "common.h"
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <lua/lua.hpp>

typedef std::int32_t lua_int;

// LuaValue hold a variety of different Lua types. We use LuaType<LuaValue> when
// we don't care exactly what type something is, but want to use it between C++
// and Lua. When we know the exact type T we're working with, we use LuaType<T>.
struct LuaValue {
  LuaValue(void* userdata, const std::string& metatable);
  LuaValue(y::world world);
  LuaValue(bool boolean);
  LuaValue(const std::string& string);
  LuaValue(const std::vector<LuaValue>& array);

  LuaValue(const LuaValue& value);
  LuaValue(LuaValue&& value);
  LuaValue& operator=(const LuaValue& value);
  LuaValue& operator=(LuaValue&& value);
  ~LuaValue();

  enum {
    USERDATA,
    WORLD,
    BOOLEAN,
    STRING,
    ARRAY,
  } type;

  void* userdata;
  y::world world;
  bool boolean;
  std::string string;
  std::vector<LuaValue> array;
};

// Base type for non-primitive (i.e. userdata) type information structures.
// Since we need to copy these objects around arbitrarily without necessarily
// knowing the type at compile-time, we need a mechanism to allow this.
struct LuaGenericType {
  virtual void to_lua(lua_State* state, const void* v) const = 0;
  virtual void* copy(const void* v) const = 0;
  virtual void del(void* v) const = 0;
};
typedef std::unordered_map<
    std::string, std::unique_ptr<const LuaGenericType>> LuaGenericTypeMap;
extern LuaGenericTypeMap lua_generic_type_map;

// Generic Lua-allocated and copied type.
template<typename T>
struct LuaType : LuaGenericType {
  // For generic types, this can't be defined automatically here. Should be
  // defined by a y_*typedef macro in lua_api.h.
  static const std::string type_name;
  static T default_value;

  void to_lua(lua_State* state, const void* v) const override;
  void* copy(const void* v) const override;
  void del(void* v) const override;

  inline T& get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, const T& arg) const;
};

// World scalar type.
template<>
struct LuaType<y::world> {
  static const std::string type_name;
  static y::world default_value;

  inline y::world get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, y::world arg) const;
};

// Integer type.
template<>
struct LuaType<std::int32_t> {
  static const std::string type_name;
  static std::int32_t default_value;

  inline std::int32_t get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, std::int32_t arg) const;
};

// Boolean type.
template<>
struct LuaType<bool> {
  static const std::string type_name;
  static bool default_value;

  inline bool get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, bool arg) const;
};

// String type.
template<>
struct LuaType<std::string> {
  static const std::string type_name;
  static std::string default_value;

  inline std::string get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, const std::string& arg) const;
};

// Array type.
template<typename T>
struct LuaType<std::vector<T>> {
  static const std::string type_name;
  static std::vector<T> default_value;

  inline std::vector<T> get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, const std::vector<T>& arg) const;
};

// Generic LuaValue union type.
template<>
struct LuaType<LuaValue> {
  static const std::string type_name;
  static LuaValue default_value;

  inline LuaValue get(lua_State* state, lua_int index) const;
  inline bool is(lua_State* state, lua_int index) const;
  inline void push(lua_State* state, const LuaValue& arg) const;
};

// Standard type names and values.
template<typename T>
const std::string LuaType<std::vector<T>>::type_name =
    "array<" + LuaType<T>::type_name + ">";

template<typename T>
T LuaType<T>::default_value;
template<typename T>
std::vector<T> LuaType<std::vector<T>>::default_value;

// Generic Lua-allocated and copied implementation.
template<typename T>
void LuaType<T>::to_lua(lua_State* state, const void* v) const
{
  typedef typename std::remove_const<T>::type T_non_const;
  T_non_const* t = reinterpret_cast<T_non_const*>(
      lua_newuserdata(state, sizeof(T)));
  const T& arg = *reinterpret_cast<const T*>(v);
  new (t) T_non_const(arg);
}

template<typename T>
void* LuaType<T>::copy(const void* v) const
{
  const T& arg = *reinterpret_cast<const T*>(v);
  return new typename std::remove_const<T>::type(arg);
}

template<typename T>
void LuaType<T>::del(void* v) const
{
  delete reinterpret_cast<T*>(v);
}

template<typename T>
T& LuaType<T>::get(lua_State* state, lua_int index) const
{
  void* v = lua_touserdata(state, index);
  T* t = reinterpret_cast<T*>(v);
  return *t;
}

template<typename T>
bool LuaType<T>::is(lua_State* state, lua_int index) const
{
  void *v = lua_touserdata(state, index);
  if (!v || !lua_getmetatable(state, index)) {
    return false;
  }

  luaL_getmetatable(state, type_name.c_str());
  if (!lua_rawequal(state, -1, -2)) {
    v = nullptr;
  }
  lua_pop(state, 2);
  return v;
}

template<typename T>
void LuaType<T>::push(lua_State* state, const T& arg) const
{
  T* t = reinterpret_cast<T*>(lua_newuserdata(state, sizeof(T)));
  luaL_getmetatable(state, type_name.c_str());
  lua_setmetatable(state, -2);
  new (t) T(arg);
}

// World scalar implementation.
y::world LuaType<y::world>::get(lua_State* state, lua_int index) const
{
  return lua_tonumber(state, index);
}

bool LuaType<y::world>::is(lua_State* state, lua_int index) const
{
  return lua_isnumber(state, index);
}

void LuaType<y::world>::push(lua_State* state, y::world arg) const
{
  lua_pushnumber(state, arg);
}

// Integer implementation.
std::int32_t LuaType<std::int32_t>::get(lua_State* state, lua_int index) const
{
  return lua_tointeger(state, index);
}

bool LuaType<std::int32_t>::is(lua_State* state, lua_int index) const
{
  return lua_isnumber(state, index);
}

void LuaType<std::int32_t>::push(lua_State* state, std::int32_t arg) const
{
  lua_pushinteger(state, arg);
}

// Boolean implementation.
bool LuaType<bool>::get(lua_State* state, lua_int index) const
{
  return lua_toboolean(state, index);
}

bool LuaType<bool>::is(lua_State* state, lua_int index) const
{
   return lua_isboolean(state, index);
}

void LuaType<bool>::push(lua_State* state, bool arg) const
{
  lua_pushboolean(state, arg);
}

// String implementation.
std::string LuaType<std::string>::get(lua_State* state, lua_int index) const
{
  return std::string(lua_tostring(state, index));
}

bool LuaType<std::string>::is(lua_State* state, lua_int index) const
{
  return lua_isstring(state, index);
}

void LuaType<std::string>::push(lua_State* state, const std::string& arg) const
{
  lua_pushstring(state, arg.c_str());
}

// Array implementation.
template<typename T>
std::vector<T> LuaType<std::vector<T>>::get(
    lua_State* state, lua_int index) const
{
  std::vector<T> t;
  LuaType<T> element_type;

  lua_int size = 1;
  for (; true; ++size) {
    lua_rawgeti(state, index, size);
    if (lua_isnil(state, lua_gettop(state))) {
      break;
    }
    t.emplace_back(element_type.get(state, lua_gettop(state)));
  }
  lua_pop(state, size);
  return t;
}

template<typename T>
bool LuaType<std::vector<T>>::is(lua_State* state, lua_int index) const
{
  if (!lua_istable(state, index)) {
    return false;
  }
  LuaType<T> element_type;

  bool b = true;
  lua_int size = 1;
  for (; true; ++size) {
    lua_rawgeti(state, index, size);
    if (lua_isnil(state, lua_gettop(state))) {
      break;
    }
    b &= element_type.is(state, lua_gettop(state));
  }
  lua_pop(state, size);
  return b;
}

template<typename T>
void LuaType<std::vector<T>>::push(
    lua_State* state, const std::vector<T>& arg) const
{
  lua_createtable(state, arg.size(), 0);
  LuaType<T> element_type;

  lua_int index = lua_gettop(state);
  for (std::size_t i = 0; i < arg.size(); ++i) {
    element_type.push(state, arg[i]);
    lua_rawseti(state, index, 1 + i);
  }
}

// Generic LuaValue union implementation.
LuaValue LuaType<LuaValue>::get(lua_State* state, lua_int index) const
{
  LuaType<y::world> world;
  if (world.is(state, index)) {
    return LuaValue(world.get(state, index));
  }

  LuaType<bool> boolean;
  if (boolean.is(state, index)) {
    return LuaValue(boolean.get(state, index));
  }

  LuaType<std::string> string;
  if (string.is(state, index)) {
    return LuaValue(string.get(state, index));
  }

  LuaType<std::vector<LuaValue>> array;
  if (array.is(state, index)) {
    return LuaValue(array.get(state, index));
  }

  // Check for arbitrary userdata.
  void *v = lua_touserdata(state, index);
  if (v && lua_getmetatable(state, index)) {
    lua_pushstring(state, "_typename");
    lua_rawget(state, -2);
    std::string t = lua_tostring(state, -1);
    lua_pop(state, 2);
    return LuaValue(lua_generic_type_map[t]->copy(v), t);
  }

  return LuaValue(0.);
}

bool LuaType<LuaValue>::is(lua_State* state, lua_int index) const
{
  LuaType<y::world> world;
  LuaType<bool> boolean;
  LuaType<std::string> string;
  LuaType<std::vector<LuaValue>> array;

  void *v = lua_touserdata(state, index);
  bool is_userdata = v && lua_getmetatable(state, index);
  if (is_userdata) {
    lua_pop(state, 1);
  }

  return
      is_userdata ||
      world.is(state, index) ||
      boolean.is(state, index) ||
      string.is(state, index) ||
      array.is(state, index);
}

void LuaType<LuaValue>::push(lua_State* state, const LuaValue& arg) const
{
  switch (arg.type) {
    case LuaValue::USERDATA:
      lua_generic_type_map[arg.string]->to_lua(state, arg.userdata);
      luaL_getmetatable(state, arg.string.c_str());
      lua_setmetatable(state, -2);
      break;
    case LuaValue::WORLD:
      LuaType<y::world>().push(state, arg.world);
      break;
    case LuaValue::BOOLEAN:
      LuaType<bool>().push(state, arg.boolean);
      break;
    case LuaValue::STRING:
      LuaType<std::string>().push(state, arg.string);
      break;
    case LuaValue::ARRAY:
      LuaType<std::vector<LuaValue>>().push(state, arg.array);
      break;
    default: {}
  }
}

#endif
