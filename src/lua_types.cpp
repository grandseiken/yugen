#include "lua_types.h"
#include "common/algorithm.h"

LuaValue::LuaValue(void* userdata, const y::string& metatable)
  : type(USERDATA)
  , userdata(userdata)
  , string(metatable)
{
}

LuaValue::LuaValue(y::world world)
  : type(WORLD)
  , userdata(y::null)
  , world(world)
{
}

LuaValue::LuaValue(bool boolean)
  : type(BOOLEAN)
  , userdata(y::null)
  , boolean(boolean)
{
}

LuaValue::LuaValue(const y::string& string)
  : type(STRING)
  , userdata(y::null)
  , string(string)
{
}

LuaValue::LuaValue(const y::vector<LuaValue>& array)
  : type(ARRAY)
  , userdata(y::null)
  , array(array)
{
}

LuaValue::LuaValue(const LuaValue& value)
  : type(WORLD)
  , userdata(y::null)
{
  operator=(value);
}

LuaValue::LuaValue(LuaValue&& value)
  : type(WORLD)
  , userdata(y::null)
{
  operator=(y::move(value));
}

LuaValue& LuaValue::operator=(const LuaValue& value)
{
  if (this == &value) {
    return *this;
  }

  if (type == USERDATA) {
    lua_generic_type_map[string]->del(userdata);
    userdata = y::null;
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
  y::swap(type, value.type);
  y::swap(userdata, value.userdata);
  y::swap(world, value.world);
  y::swap(boolean, value.boolean);
  y::swap(string, value.string);
  y::swap(array, value.array);
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
const y::string LuaType<y::world>::type_name = "world";
const y::string LuaType<y::int32>::type_name = "int";
const y::string LuaType<bool>::type_name = "bool";
const y::string LuaType<y::string>::type_name = "string";
const y::string LuaType<LuaValue>::type_name = "[value]";

y::world LuaType<y::world>::default_value = 0.;
y::int32 LuaType<y::int32>::default_value = 0;
bool LuaType<bool>::default_value = false;
y::string LuaType<y::string>::default_value = "";
LuaValue LuaType<LuaValue>::default_value(0.);
