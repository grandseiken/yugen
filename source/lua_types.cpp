#include "lua_types.h"

LuaValue::LuaValue(y::world world)
  : type(WORLD)
  , world(world)
{
}

LuaValue::LuaValue(bool boolean)
  : type(BOOLEAN)
  , boolean(boolean)
{
}

LuaValue::LuaValue(const y::string& string)
  : type(STRING)
  , string(string)
{
}

LuaValue::LuaValue(const y::vector<LuaValue>& array)
  : type(ARRAY)
  , array(array)
{
}

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
