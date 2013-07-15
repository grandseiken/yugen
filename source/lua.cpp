#include "lua.h"

// Variadic push.
lua_int push_all(lua_State* state)
{
  (void)state;
  return 0;
}

template<typename T, typename... U>
lua_int push_all(lua_State* state, const T& arg, const U&... args)
{
  LuaType<T> t;
  t.push(state, arg);
  push_all(state, args...);
  return 1 + sizeof...(U);
}

// Generic get.
template<typename T>
auto lua_get(lua_State* state, lua_int index) ->
    decltype(LuaType<T>().get(state, index))
{
  LuaType<T> t;
#ifdef LUA_DEBUG
  luaL_argcheck(state, t.is(state, index), index,
                (LuaType<T>::type_name + " expected").c_str());
#endif
  return t.get(state, index);
}

struct RegistryIndex {};
static RegistryIndex stage_registry_index;

/******************************************************************************/
/**** Preprocessor magic for defining types and functions useable by Lua   ****/
/**** in a style that sort of approximates C functions:                    ****/
/******************************************************************************/
/***/ #define y_ptrtypedef(T)                                                  \
/***/ template<>                                                               \
/***/ const y::string LuaType<T*>::type_name = "y." #T "*";                    \
/***/ template<>                                                               \
/***/ const y::string LuaType<const T*>::type_name = "y." #T "*";              \
/***/ y_api(T##_eq) y_arg(T*, a) y_arg(T*, b)                                  \
/***/ {                                                                        \
/***/   y_return(a == b);                                                      \
/***/ } void _y_typedef_##T##_no_op()
/***/
/***/ #define y_valtypedef(T)                                                  \
/***/ template<>                                                               \
/***/ const y::string LuaType<T>::type_name = "y." #T;                         \
/***/ template<>                                                               \
/***/ const y::string LuaType<const T>::type_name = "y." #T;                   \
/***/ namespace _y_api_##T {                                                   \
/***/   void _y_typedef(lua_State* _y_state)                                   \
/***/   {                                                                      \
/***/     luaL_newmetatable(_y_state, LuaType<T>::type_name.c_str());          \
/***/     lua_pushstring(_y_state, "__index");                                 \
/***/     lua_pushvalue(_y_state, -2);                                         \
/***/     lua_settable(_y_state, -3);
/***/
/***/ #define y_method(name, function)                                         \
/***/     lua_pushstring(_y_state, name);                                      \
/***/     lua_pushcfunction(_y_state,                                          \
/***/                       _y_api_##function::_y_ref);                        \
/***/     lua_settable(_y_state, -3);
/***/
/***/ #define y_endtypedef()                                                   \
/***/   }                                                                      \
/***/ } void _y_typedef_##T##_no_op()
/***/
/***/ #define y_api(name)                                                      \
/***/ namespace _y_api_##name {                                                \
/***/   lua_int _y_api(lua_State* _y_state);                                   \
/***/   typedef lua_int _y_api_type(lua_State* _y_state);                      \
/***/   static const char* const _y_name = #name;                              \
/***/   static _y_api_type* const _y_ref = _y_api;                             \
/***/   lua_int _y_api(lua_State* _y_state)                                    \
/***/   {                                                                      \
/***/     lua_int _y_stack = 0;                                                \
/***/     (void)_y_stack;                                                      \
/***/     lua_pushlightuserdata(                                               \
/***/         _y_state,                                                        \
/***/         reinterpret_cast<void*>(&stage_registry_index));                 \
/***/     lua_gettable(_y_state, LUA_REGISTRYINDEX);                           \
/***/     GameStage& stage = *lua_get<GameStage*>(_y_state, -1);               \
/***/     lua_pop(_y_state, 1);                                                \
/***/     (void)stage;
/***/
/***/ #define y_arg(T, name)                                                   \
/***/     decltype(lua_get<T>(_y_state, _y_stack)) name =                      \
/***/         lua_get<T>(_y_state, ++_y_stack);
/***/
/***/ #define y_return(...)                                                    \
/***/       return push_all(_y_state, __VA_ARGS__);                            \
/***/     }                                                                    \
/***/   } void _y_no_op()
/***/
/***/ #define y_void()                                                         \
/***/       return 0;                                                          \
/***/     }                                                                    \
/***/   } void _y_no_op()
/******************************************************************************/
#include "lua_api.h"
/******************************************************************************/
/**** Preprocessor magic for enumerating the library of functions and      ****/
/**** types and storing them in a Lua stack:                               ****/
/******************************************************************************/
/***/ #undef y_ptrtypedef
/***/ #undef y_valtypedef
/***/ #undef y_method
/***/ #undef y_endtypedef
/***/ #undef y_api
/***/ #undef y_arg
/***/ #undef y_refarg
/***/ #undef y_checked_arg
/***/ #undef y_return
/***/ #undef y_void
/***/
/***/ #define y_ptrtypedef(T)                                                  \
/***/   luaL_newmetatable(_y_state, LuaType<T*>::type_name.c_str());           \
/***/   lua_pushstring(_y_state, "__eq");                                      \
/***/   lua_pushcfunction(_y_state,                                            \
/***/                     _y_api_##T##_eq::_y_ref);                            \
/***/   lua_settable(_y_state, -3);                                            \
/***/   lua_pop(_y_state, 1)
/***/
/***/ #define y_valtypedef(T)                                                  \
/***/   _y_api_##T::_y_typedef(_y_state);                                      \
/***/   lua_pop(_y_state, 1);                                                  \
/***/   (void)LuaType<T>::type_name
/***/
/***/ #define y_method(name, function)
/***/
/***/ #define y_endtypedef()
/***/
/***/ #define y_api(name)                                                      \
/***/   lua_register(_y_state,                                                 \
/***/                _y_api_##name::_y_name,                                   \
/***/                _y_api_##name::_y_ref);                                   \
/***/   struct _y_no_op_struct_##name {                                        \
/***/     static void no_op()                                                  \
/***/     {                                                                    \
/***/       GameStage& stage = *(GameStage*)y::null;                           \
/***/       (void)stage;
/***/
/***/ #define y_arg(T, name)                                                   \
/***/       decltype(lua_get<T>(y::null, 0)) name =                            \
/***/           lua_get<T>(y::null, 0);                                        \
/***/       (void)name;
/***/
/***/ #define y_return(...)                                                    \
/***/       }                                                                  \
/***/     }                                                                    \
/***/   }; while (false) {(void)0
/***/
/***/ #define y_void() y_return()
/******************************************************************************/
void y_register(lua_State* _y_state)
{
#include "lua_api.h"
}
/******************************************************************************/
/***/ #undef y_ptrtypedef
/***/ #undef y_valtypedef
/***/ #undef y_method
/***/ #undef y_endtypedef
/***/ #undef y_api
/***/ #undef y_arg
/***/ #undef y_refarg
/***/ #undef y_checked_arg
/***/ #undef y_return
/***/ #undef y_void
/******************************************************************************/
/**** End of proprocessor magic.                                           ****/
/******************************************************************************/

ScriptReference::ScriptReference(Script& script)
  : _script(&script)
{
  script._reference_set.insert(this);
}

ScriptReference::ScriptReference(const ScriptReference& script)
  : _script(script._script)
{
  if (_script) {
    _script->_reference_set.insert(this);
  }
}

ScriptReference::~ScriptReference()
{
  if (is_valid()) {
    _script->_reference_set.erase(this);
  }
}

ScriptReference& ScriptReference::operator=(const ScriptReference& arg)
{
  if (this == &arg) {
    return *this;
  }
  if (_script) {
    _script->_reference_set.erase(this);
  }
  _script = arg._script;
  if (_script) {
    _script->_reference_set.insert(this);
  }
  return *this;
}

bool ScriptReference::operator==(const ScriptReference& arg)
{
  return _script == arg._script;
}

bool ScriptReference::operator!=(const ScriptReference& arg)
{
  return !operator==(arg);
}

bool ScriptReference::is_valid() const
{
  return _script && !_script->is_destroyed();
}

void ScriptReference::invalidate()
{
  _script = y::null;
}

const Script* ScriptReference::get() const
{
  return _script;
}

Script* ScriptReference::get()
{
  return _script;
}

const Script& ScriptReference::operator*() const
{
  return *_script;
}

Script& ScriptReference::operator*()
{
  return *_script;
}

const Script* ScriptReference::operator->() const
{
  return _script;
}

Script* ScriptReference::operator->()
{
  return _script;
}

ConstScriptReference::ConstScriptReference(const Script& script)
: _script(&script)
{
  script._const_reference_set.insert(this);
}

ConstScriptReference::ConstScriptReference(const ScriptReference& script)
  : _script(script.get())
{
  if (_script) {
    _script->_const_reference_set.insert(this);
  }
}

ConstScriptReference::ConstScriptReference(const ConstScriptReference& script)
  : _script(script._script)
{
  if (_script) {
    _script->_const_reference_set.insert(this);
  }
}

ConstScriptReference::~ConstScriptReference()
{
  if (is_valid()) {
    _script->_const_reference_set.erase(this);
  }
}

ConstScriptReference& ConstScriptReference::operator=(
    const ConstScriptReference& arg)
{
  if (this == &arg) {
    return *this;
  }
  if (_script) {
    _script->_const_reference_set.erase(this);
  }
  _script = arg._script;
  if (_script) {
    _script->_const_reference_set.insert(this);
  }
  return *this;
}

bool ConstScriptReference::operator==(const ConstScriptReference& arg)
{
  return _script == arg._script;
}

bool ConstScriptReference::operator!=(const ConstScriptReference& arg)
{
  return !operator==(arg);
}

bool ConstScriptReference::is_valid() const
{
  return _script && !_script->is_destroyed();
}

void ConstScriptReference::invalidate()
{
  _script = y::null;
}

const Script* ConstScriptReference::get() const
{
  return _script;
}

const Script& ConstScriptReference::operator*() const
{
  return *_script;
}

const Script* ConstScriptReference::operator->() const
{
  return _script;
}

Script::Script(GameStage& stage,
               const y::string& path, const y::string& contents,
               const y::wvec2& origin, const y::wvec2& region)
  : _path(path)
  // Use standard allocator and panic function.
  , _state(luaL_newstate())
  , _origin(origin)
  , _region(region)
  , _rotation(0.)
  , _destroyed(false)
{
  // Load the Lua standard library.
  luaL_openlibs(_state);
  // Register Lua API.
  y_register(_state);

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

  // Use traceback error handler (stored in stack at position 1 always).
  lua_getglobal(_state, "debug");
  lua_getfield(_state, 1, "traceback");
  lua_remove(_state, 1);

  // Set GameStage reference in the registry.
  lua_pushlightuserdata(
      _state, reinterpret_cast<void*>(&stage_registry_index));
  push_all(_state, &stage);
  lua_settable(_state, LUA_REGISTRYINDEX);

  // Set self-reference global variable.
  push_all(_state, this);
  lua_setglobal(_state, "self");

  local data_struct{contents, false};
  if (lua_load(_state, local::read, &data_struct, _path.c_str(), y::null) ||
      lua_pcall(_state, 0, 0, 1)) {
    const char* error = lua_tostring(_state, -1);
    std::cerr << "Loading script " << _path << " failed";
    if (error) {
      std::cerr << ": " << error;
    }
    std::cerr << std::endl;
  }
}

Script::~Script()
{
  lua_close(_state);
  for (ScriptReference* ref : _reference_set) {
    ref->invalidate();
  }
  for (ConstScriptReference* ref : _const_reference_set) {
    ref->invalidate();
  }
}

const y::string& Script::get_path() const
{
  return _path;
}

const y::wvec2& Script::get_origin() const
{
  return _origin;
}

const y::wvec2& Script::get_region() const
{
  return _region;
}

y::world Script::get_rotation() const
{
  return _rotation;
}

void Script::set_origin(const y::wvec2& origin)
{
  _origin = origin;
}

void Script::set_region(const y::wvec2& region)
{
  _region = region;
}

void Script::set_rotation(y::world rotation)
{
  _rotation = rotation;
}

bool Script::has_function(const y::string& function_name) const
{
  lua_getglobal(_state, function_name.c_str());
  bool has = lua_isfunction(_state, -1);
  lua_pop(_state, 1);
  return has;
}

void Script::call(const y::string& function_name)
{
  lua_getglobal(_state, function_name.c_str());
  if (lua_pcall(_state, 0, 0, 1)) {
    const char* error = lua_tostring(_state, -1);
    std::cerr << "Calling function " << _path << ":" <<
        function_name << " failed";
    if (error) {
      std::cerr << ": " << error;
    }
    std::cerr << std::endl;
  }
}

void Script::call(const y::string& function_name,
                  const y::vector<LuaValue>& args)
{
  lua_getglobal(_state, function_name.c_str());
  LuaType<LuaValue> t;
  for (const LuaValue& arg : args) {
    t.push(_state, arg);
  }
  if (lua_pcall(_state, args.size(), 0, 1)) {
    const char* error = lua_tostring(_state, -1);
    std::cerr << "Calling function " << _path << ":" <<
        function_name << " failed";
    if (error) {
      std::cerr << ": " << error;
    }
    std::cerr << std::endl;
  }
}

void Script::destroy()
{
  _destroyed = true;
}

bool Script::is_destroyed() const
{
  return _destroyed;
}
