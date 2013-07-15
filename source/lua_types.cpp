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

// Standard type names.
const y::string LuaType<y::world>::type_name = "world";
const y::string LuaType<y::int32>::type_name = "int";
const y::string LuaType<bool>::type_name = "bool";
const y::string LuaType<y::string>::type_name = "string";
const y::string LuaType<LuaValue>::type_name = "LuaValue";
