#include "lua.h"

namespace {
  // Variadic push.
  lua_int push_all(lua_State*)
  {
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

  // Generic is and get functions.
  template<typename T>
  bool lua_is(lua_State* state, lua_int index)
  {
    return LuaType<T>().is(state, index);
  }

  template<typename T>
  auto lua_get(lua_State* state, lua_int index) ->
      decltype(LuaType<T>().get(state, index))
  {
#ifdef DEBUG
    luaL_argcheck(state, lua_is<T>(state, index), index,
                  (LuaType<T>::type_name + " expected").c_str());
#endif
    return LuaType<T>().get(state, index);
  }

  // Lua assert.
  void lua_argassert(lua_State* state,
                     bool condition, lua_int index, const std::string& message)
  {
#ifdef DEBUG
    luaL_argcheck(state, condition, index, message.c_str());
#else
    (void)state;
    (void)condition;
    (void)message;
#endif
  }

  struct RegistryIndex {};
  static RegistryIndex stage_registry_index;
}

/******************************************************************************/
/**** Preprocessor magic for defining types and functions useable by Lua   ****/
/**** in a style that sort of approximates C functions:                    ****/
/******************************************************************************/
/***/ #define y_ptrtypedef(T)                                                  \
/***/ template<>                                                               \
/***/ const std::string LuaType<T*>::type_name = "y." #T "*";                  \
/***/ template<>                                                               \
/***/ const std::string LuaType<const T*>::type_name = "y." #T "*";            \
/***/ y_api(T##__eq) y_arg(const T*, a) y_arg(const T*, b)                     \
/***/ {                                                                        \
/***/   y_return(a == b);                                                      \
/***/ }                                                                        \
/***/ namespace _y_api_##T {                                                   \
/***/   void _y_typedef(lua_State* _y_state)                                   \
/***/   {                                                                      \
/***/     luaL_newmetatable(_y_state, LuaType<T*>::type_name.c_str());         \
/***/     lua_pushstring(_y_state, "__index");                                 \
/***/     lua_pushvalue(_y_state, -2);                                         \
/***/     lua_settable(_y_state, -3);                                          \
/***/     lua_pushstring(_y_state, "_typename");                               \
/***/     lua_pushstring(_y_state, LuaType<T*>::type_name.c_str());            \
/***/     lua_settable(_y_state, -3);                                          \
/***/     y_method("__eq", T##__eq);
/***/
/***/ #define y_valtypedef(T)                                                  \
/***/ template<>                                                               \
/***/ const std::string LuaType<T>::type_name = "y." #T;                       \
/***/ template<>                                                               \
/***/ const std::string LuaType<const T>::type_name = "y." #T;                 \
/***/ y_api(T##__eq) y_arg(const T, a) y_arg(const T, b)                       \
/***/ {                                                                        \
/***/   y_return(a == b);                                                      \
/***/ }                                                                        \
/***/ namespace _y_api_##T {                                                   \
/***/   void _y_typedef(lua_State* _y_state)                                   \
/***/   {                                                                      \
/***/     luaL_newmetatable(_y_state, LuaType<T>::type_name.c_str());          \
/***/     lua_pushstring(_y_state, "__index");                                 \
/***/     lua_pushvalue(_y_state, -2);                                         \
/***/     lua_settable(_y_state, -3);                                          \
/***/     lua_pushstring(_y_state, "_typename");                               \
/***/     lua_pushstring(_y_state, LuaType<T>::type_name.c_str());             \
/***/     lua_settable(_y_state, -3);                                          \
/***/     y_method("__eq", T##__eq);
/***/
/***/ #define y_method(name, function)                                         \
/***/     lua_pushstring(_y_state, name);                                      \
/***/     lua_pushcfunction(_y_state,                                          \
/***/                       _y_api_##function::_y_ref);                        \
/***/     lua_settable(_y_state, -3)
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
/***/ #define y_varargs(T, name)                                               \
/***/     std::vector<T> name;                                                 \
/***/     while (++_y_stack <= lua_gettop(_y_state)) {                         \
/***/       name.emplace_back(lua_get<T>(_y_state, _y_stack));                 \
/***/     }
/***/
/***/ #define y_optarg(T, name)                                                \
/***/     bool name##_defined = _y_stack < lua_gettop(_y_state) &&             \
/***/         lua_is<T>(_y_state, 1 + _y_stack);                               \
/***/     (void)name##_defined;                                                \
/***/     decltype(lua_get<T>(_y_state, _y_stack)) name =                      \
/***/         name##_defined ? lua_get<T>(_y_state, ++_y_stack)                \
/***/                        : LuaType<T>::default_value;
/***/
/***/ #define y_assert(condition, index, message)                              \
/***/     lua_argassert((_y_state), (condition), (index), (message))
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
/***/ #undef y_namespace
/***/ #undef y_ptrtypedef
/***/ #undef y_valtypedef
/***/ #undef y_method
/***/ #undef y_endtypedef
/***/ #undef y_api
/***/ #undef y_arg
/***/ #undef y_varargs
/***/ #undef y_optarg
/***/ #undef y_assert
/***/ #undef y_return
/***/ #undef y_void
/***/
/***/ #define y_namespace()
/***/
/***/ #define y_ptrtypedef(T)                                                  \
/***/   _y_api_##T::_y_typedef(_y_state);                                      \
/***/   lua_pop(_y_state, 1);                                                  \
/***/   if (lua_generic_type_map.find(LuaType<T*>::type_name) ==               \
/***/       lua_generic_type_map.end()) {                                      \
/***/     lua_generic_type_map.emplace(                                        \
/***/         LuaType<T*>::type_name,                                          \
/***/         std::unique_ptr<LuaGenericType>(new LuaType<T*>()));             \
/***/   }                                                                      \
/***/   (void)LuaType<T*>::type_name;
/***/
/***/ #define y_valtypedef(T)                                                  \
/***/   _y_api_##T::_y_typedef(_y_state);                                      \
/***/   lua_pop(_y_state, 1);                                                  \
/***/   if (lua_generic_type_map.find(LuaType<T>::type_name) ==                \
/***/       lua_generic_type_map.end()) {                                      \
/***/     lua_generic_type_map.emplace(                                        \
/***/         LuaType<T>::type_name,                                           \
/***/         std::unique_ptr<LuaGenericType>(new LuaType<T>()));              \
/***/   }                                                                      \
/***/   (void)LuaType<T>::type_name;
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
/***/       GameStage& stage = *(GameStage*)nullptr;                           \
/***/       (void)stage;
/***/
/***/ #define y_arg(T, name)                                                   \
/***/       decltype(lua_get<T>(nullptr, 0)) name = lua_get<T>(nullptr, 0);    \
/***/       (void)name;
/***/
/***/ #define y_varargs(T, name)                                               \
/***/       std::vector<T> name;                                                 \
/***/       (void)name;
/***/
/***/ #define y_optarg(T, name)                                                \
/***/       bool name##_defined = false;                                       \
/***/       decltype(lua_get<T>(nullptr, 0)) name = lua_get<T>(nullptr, 0);    \
/***/       (void)name##_defined;                                              \
/***/       (void)name;
/***/
/***/ #define y_assert(condition, index, message)
/***/
/***/ #define y_return(...)                                                    \
/***/         push_all(0, __VA_ARGS__);                                        \
/***/       }                                                                  \
/***/     }                                                                    \
/***/   }; while (false) {(void)0
/***/
/***/ #define y_void()                                                         \
/***/         ;                                                                \
/***/       }                                                                  \
/***/     }                                                                    \
/***/   }; while (false) {(void)0
/******************************************************************************/
namespace {
  void y_register(lua_State* _y_state)
  {
#include "lua_api.h"
  }
}
/******************************************************************************/
/***/ #undef y_namespace
/***/ #undef y_ptrtypedef
/***/ #undef y_valtypedef
/***/ #undef y_method
/***/ #undef y_endtypedef
/***/ #undef y_api
/***/ #undef y_arg
/***/ #undef y_varargs
/***/ #undef y_optarg
/***/ #undef y_assert
/***/ #undef y_return
/***/ #undef y_void
/******************************************************************************/
/**** End of proprocessor magic.                                           ****/
/******************************************************************************/

ScriptReference::ScriptReference(Script& script)
  : _script(&script)
  , _callback_id(_script->add_destroy_callback(
        std::bind(&ScriptReference::invalidate, this, std::placeholders::_1)))
{
}

ScriptReference::ScriptReference(const ScriptReference& script)
  : _script(script._script)
  , _callback_id(-1)
{
  if (_script) {
    _callback_id = _script->add_destroy_callback(
        std::bind(&ScriptReference::invalidate, this, std::placeholders::_1));
  }
}

ScriptReference::~ScriptReference()
{
  if (is_valid()) {
    _script->remove_destroy_callback(_callback_id);
  }
}

ScriptReference& ScriptReference::operator=(const ScriptReference& arg)
{
  if (this == &arg) {
    return *this;
  }
  if (_script) {
    _script->remove_destroy_callback(_callback_id);
  }
  _script = arg._script;
  if (_script) {
    _callback_id = _script->add_destroy_callback(
        std::bind(&ScriptReference::invalidate, this, std::placeholders::_1));
  }
  return *this;
}

bool ScriptReference::operator==(const ScriptReference& arg) const
{
  return _script == arg._script;
}

bool ScriptReference::operator!=(const ScriptReference& arg) const
{
  return !operator==(arg);
}

bool ScriptReference::is_valid() const
{
  return _script && !_script->is_destroyed();
}

void ScriptReference::invalidate(Script* script)
{
  (void)script;
  _script = nullptr;
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
  , _callback_id(_script->add_destroy_callback(
        std::bind(&ConstScriptReference::invalidate,
                  this, std::placeholders::_1)))
{
}

ConstScriptReference::ConstScriptReference(const ScriptReference& script)
  : _script(script.get())
{
  if (_script) {
    _callback_id = _script->add_destroy_callback(
        std::bind(&ConstScriptReference::invalidate,
                  this, std::placeholders::_1));
  }
}

ConstScriptReference::ConstScriptReference(const ConstScriptReference& script)
  : _script(script._script)
{
  if (_script) {
    _callback_id = _script->add_destroy_callback(
        std::bind(&ConstScriptReference::invalidate, this,
                  std::placeholders::_1));
  }
}

ConstScriptReference::~ConstScriptReference()
{
  if (is_valid()) {
    _script->remove_destroy_callback(_callback_id);
  }
}

ConstScriptReference& ConstScriptReference::operator=(
    const ConstScriptReference& arg)
{
  if (this == &arg) {
    return *this;
  }
  if (_script) {
    _script->remove_destroy_callback(_callback_id);
  }
  _script = arg._script;
  if (_script) {
    _callback_id = _script->add_destroy_callback(
        std::bind(&ConstScriptReference::invalidate,
                  this, std::placeholders::_1));
  }
  return *this;
}

bool ConstScriptReference::operator==(const ConstScriptReference& arg) const
{
  return _script == arg._script;
}

bool ConstScriptReference::operator!=(const ConstScriptReference& arg) const
{
  return !operator==(arg);
}

bool ConstScriptReference::is_valid() const
{
  return _script && !_script->is_destroyed();
}

void ConstScriptReference::invalidate(Script* script)
{
  (void)script;
  _script = nullptr;
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
               const std::string& path, const std::string& contents,
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
  struct read_data {
    const std::string& data;
    bool has_read;
  };

  auto read = [&](lua_State*, void* data, std::size_t* size)
  {
    read_data* data_struct = reinterpret_cast<read_data*>(data);
    if (data_struct->has_read) {
      *size = 0;
      return (const char*)nullptr;
    }
    data_struct->has_read = true;
    *size = data_struct->data.length();
    return data_struct->data.c_str();
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

  read_data data_struct{contents, false};
  if (lua_load(_state, read, &data_struct, _path.c_str()) ||
      lua_pcall(_state, 0, 0, 1)) {
    const char* error = lua_tostring(_state, -1);
    logg_err("Loading script ", _path, " failed");
    if (error) {
      logg_err(": ", error);
    }
    log_err();
  }
}

Script::~Script()
{
  destroy();
  lua_close(_state);
}

const std::string& Script::get_path() const
{
  return _path;
}

const y::wvec2& Script::get_region() const
{
  return _region;
}

const y::wvec2& Script::get_origin() const
{
  return _origin;
}

y::world Script::get_rotation() const
{
  return _rotation;
}

void Script::set_region(const y::wvec2& region)
{
  _region = region;
}

void Script::set_origin(const y::wvec2& origin)
{
  _origin = origin;
  _move_callbacks(this);
}

void Script::set_rotation(y::world rotation)
{
  _rotation = rotation;
  _move_callbacks(this);
}

bool Script::has_function(const std::string& function_name) const
{
  lua_getglobal(_state, function_name.c_str());
  bool has = lua_isfunction(_state, -1);
  lua_pop(_state, 1);
  return has;
}

void Script::call(const std::string& function_name, const lua_args& args)
{
  lua_args output;
  call(output, function_name, args);
}

void Script::call(lua_args& output, const std::string& function_name,
                  const lua_args& args)
{
  lua_int top = lua_gettop(_state);
  lua_getglobal(_state, function_name.c_str());
  LuaType<LuaValue> t;
  for (const LuaValue& arg : args) {
    t.push(_state, arg);
  }
  if (lua_pcall(_state, args.size(), LUA_MULTRET, 1)) {
    const char* error = lua_tostring(_state, -1);
    logg_err("Calling function ", _path, ":", function_name, " failed");
    if (error) {
      logg_err(": ", error);
    }
    log_err();
  }
  for (++top; top <= lua_gettop(_state); ++top) {
    output.emplace_back(t.get(_state, top));
  }
}

std::int32_t Script::add_move_callback(const callback& c) const
{
  return _move_callbacks.add(c);
}

void Script::remove_move_callback(std::int32_t id) const
{
  _move_callbacks.remove(id);
}

std::int32_t Script::add_destroy_callback(const callback& c) const
{
  return _destroy_callbacks.add(c);
}

void Script::remove_destroy_callback(std::int32_t id) const
{
  _destroy_callbacks.remove(id);
}

void Script::destroy()
{
  if (_destroyed) {
    return;
  }
  _destroyed = true;
  _destroy_callbacks(this);
}

bool Script::is_destroyed() const
{
  return _destroyed;
}
