#include "lua_types.h"

LuaValue::LuaValue(void* userdata, const std::string& metatable)
  : type(USERDATA)
  , userdata(userdata)
  , string(metatable)
{
}

LuaValue::LuaValue(y::world world)
  : type(WORLD)
  , userdata(nullptr)
  , world(world)
{
}

LuaValue::LuaValue(bool boolean)
  : type(BOOLEAN)
  , userdata(nullptr)
  , boolean(boolean)
{
}

LuaValue::LuaValue(const std::string& string)
  : type(STRING)
  , userdata(nullptr)
  , string(string)
{
}

LuaValue::LuaValue(const std::vector<LuaValue>& array)
  : type(ARRAY)
  , userdata(nullptr)
  , array(array)
{
}

LuaValue::LuaValue(const LuaValue& value)
  : type(WORLD)
  , userdata(nullptr)
{
  operator=(value);
}

LuaValue::LuaValue(LuaValue&& value)
  : type(WORLD)
  , userdata(nullptr)
{
  operator=(value);
}

LuaValue& LuaValue::operator=(const LuaValue& value)
{
  if (this == &value) {
    return *this;
  }

  if (type == USERDATA) {
    lua_generic_type_map[string]->del(userdata);
    userdata = nullptr;
  }
  type = value.type;

  switch (type) {
    case USERDATA:
      string = value.string;
      userdata = lua_generic_type_map[string]->copy(value.userdata);
      break;
    case WORLD:
      world = value.world;
      break;
    case BOOLEAN:
      boolean = value.boolean;
      break;
    case STRING:
      string = value.string;
      break;
    case ARRAY:
      array = value.array;
      break;
  }
  return *this;
}

LuaValue& LuaValue::operator=(LuaValue&& value)
{
  std::swap(type, value.type);
  std::swap(userdata, value.userdata);
  std::swap(world, value.world);
  std::swap(boolean, value.boolean);
  std::swap(string, value.string);
  std::swap(array, value.array);
  return *this;
}

LuaValue::~LuaValue()
{
  if (type == USERDATA) {
    lua_generic_type_map[string]->del(userdata);
  }
}

LuaGenericTypeMap lua_generic_type_map;

// Standard type names and values.
const std::string LuaType<y::world>::type_name = "world";
const std::string LuaType<std::int32_t>::type_name = "int";
const std::string LuaType<bool>::type_name = "bool";
const std::string LuaType<std::string>::type_name = "string";
const std::string LuaType<LuaValue>::type_name = "[value]";

y::world LuaType<y::world>::default_value = 0.;
std::int32_t LuaType<std::int32_t>::default_value = 0;
bool LuaType<bool>::default_value = false;
std::string LuaType<std::string>::default_value = "";
LuaValue LuaType<LuaValue>::default_value(0.);
