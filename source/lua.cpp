#include "lua.h"

// Variadic push.
ylib_int push_all(lua_State* state)
{
  (void)state;
  return 0;
}

template<typename T, typename... U>
ylib_int push_all(lua_State* state, const T& arg, const U&... args)
{
  LuaType<T> t;
  t.push(state, arg);
  push_all(state, args...);
  return 1 + sizeof...(U);
}

// Generic get.
template<typename T>
auto ylib_get(lua_State* state, ylib_int index) ->
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
/***/ #define ylib_ptrtypedef(T)                                           /**/\
/***/ template<>                                                           /**/\
/***/ const y::string LuaType<T*>::type_name = "y." #T "*";                /**/\
/***/ template<>                                                           /**/\
/***/ const y::string LuaType<const T*>::type_name = "y." #T "*";          /**/\
/***/ ylib_api(T##_eq) ylib_arg(T*, a) ylib_arg(T*, b)                     /**/\
/***/ {                                                                    /**/\
/***/   ylib_return(a == b);                                               /**/\
/***/ } void _ylib_typedef_##T##_no_op()                                   /***/
/***/ #define ylib_valtypedef(T)                                           /**/\
/***/ template<>                                                           /**/\
/***/ const y::string LuaType<T>::type_name = "y." #T;                     /**/\
/***/ template<>                                                           /**/\
/***/ const y::string LuaType<const T>::type_name = "y." #T;               /**/\
/***/ namespace _ylib_api_##T {                                            /**/\
/***/   void _ylib_typedef(lua_State* _ylib_state)                         /**/\
/***/   {                                                                  /**/\
/***/     luaL_newmetatable(_ylib_state, LuaType<T>::type_name.c_str());   /**/\
/***/     lua_pushstring(_ylib_state, "__index");                          /**/\
/***/     lua_pushvalue(_ylib_state, -2);                                  /**/\
/***/     lua_settable(_ylib_state, -3);                                   /***/
/***/ #define ylib_method(name, function)                                  /**/\
/***/     lua_pushstring(_ylib_state, name);                               /**/\
/***/     lua_pushcfunction(_ylib_state,                                   /**/\
/***/                       _ylib_api_##function::_ylib_ref);              /**/\
/***/     lua_settable(_ylib_state, -3);                                   /***/
/***/ #define ylib_endtypedef()                                            /**/\
/***/   }                                                                  /**/\
/***/ } void _ylib_typedef_##T##_no_op()                                   /***/
/***/ #define ylib_api(name)                                               /**/\
/***/ namespace _ylib_api_##name {                                         /**/\
/***/   ylib_int _ylib_api(lua_State* _ylib_state);                        /**/\
/***/   typedef ylib_int _ylib_api_type(lua_State* _ylib_state);           /**/\
/***/   static const char* const _ylib_name = #name;                       /**/\
/***/   static _ylib_api_type* const _ylib_ref = _ylib_api;                /**/\
/***/   ylib_int _ylib_api(lua_State* _ylib_state)                         /**/\
/***/   {                                                                  /**/\
/***/     ylib_int _ylib_stack = 0;                                        /**/\
/***/     (void)_ylib_stack;                                               /**/\
/***/     lua_pushlightuserdata(                                           /**/\
/***/         _ylib_state,                                                 /**/\
/***/         reinterpret_cast<void*>(&stage_registry_index));             /**/\
/***/     lua_gettable(_ylib_state, LUA_REGISTRYINDEX);                    /**/\
/***/     GameStage& stage = *ylib_get<GameStage*>(_ylib_state, -1);       /**/\
/***/     lua_pop(_ylib_state, 1);                                         /**/\
/***/     (void)stage;                                                     /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/     decltype(ylib_get<T>(_ylib_state, _ylib_stack)) name =           /**/\
/***/         ylib_get<T>(_ylib_state, ++_ylib_stack);                     /***/
/***/ #define ylib_return(...)                                             /**/\
/***/       return push_all(_ylib_state, __VA_ARGS__);                     /**/\
/***/     }                                                                /**/\
/***/   } void _ylib_no_op()                                               /***/
/***/ #define ylib_void()                                                  /**/\
/***/       return 0;                                                      /**/\
/***/     }                                                                /**/\
/***/   } void _ylib_no_op()                                               /***/
/******************************************************************************/
#include "lua_api.h"
/******************************************************************************/
/**** Preprocessor magic for enumerating the library of functions and      ****/
/**** types and storing them in a Lua stack:                               ****/
/******************************************************************************/
/***/ #undef ylib_ptrtypedef                                               /***/
/***/ #undef ylib_valtypedef                                               /***/
/***/ #undef ylib_method                                                   /***/
/***/ #undef ylib_endtypedef                                               /***/
/***/ #undef ylib_api                                                      /***/
/***/ #undef ylib_arg                                                      /***/
/***/ #undef ylib_refarg                                                   /***/
/***/ #undef ylib_checked_arg                                              /***/
/***/ #undef ylib_return                                                   /***/
/***/ #undef ylib_void                                                     /***/
/***/ #define ylib_ptrtypedef(T)                                           /**/\
/***/   luaL_newmetatable(_ylib_state, LuaType<T*>::type_name.c_str());    /**/\
/***/   lua_pushstring(_ylib_state, "__eq");                               /**/\
/***/   lua_pushcfunction(_ylib_state,                                     /**/\
/***/                     _ylib_api_##T##_eq::_ylib_ref);                  /**/\
/***/   lua_settable(_ylib_state, -3);                                     /**/\
/***/   lua_pop(_ylib_state, 1)                                            /***/
/***/ #define ylib_valtypedef(T)                                           /**/\
/***/   _ylib_api_##T::_ylib_typedef(_ylib_state);                         /**/\
/***/   lua_pop(_ylib_state, 1);                                           /**/\
/***/   (void)LuaType<T>::type_name                                        /***/
/***/ #define ylib_method(name, function)                                  /***/
/***/ #define ylib_endtypedef()                                            /***/
/***/ #define ylib_api(name)                                               /**/\
/***/   lua_register(_ylib_state,                                          /**/\
/***/                _ylib_api_##name::_ylib_name,                         /**/\
/***/                _ylib_api_##name::_ylib_ref);                         /**/\
/***/   struct _ylib_no_op_struct_##name {                                 /**/\
/***/     static void no_op()                                              /**/\
/***/     {                                                                /**/\
/***/       GameStage& stage = *(GameStage*)y::null;                       /**/\
/***/       (void)stage;                                                   /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/       decltype(ylib_get<T>(y::null, 0)) name =                       /**/\
/***/           ylib_get<T>(y::null, 0);                                   /**/\
/***/       (void)name;                                                    /***/
/***/ #define ylib_return(...)                                             /**/\
/***/       }                                                              /**/\
/***/     }                                                                /**/\
/***/   }; while (false) {(void)0                                          /***/
/***/ #define ylib_void() ylib_return()                                    /***/
/***/ void ylib_register(lua_State* _ylib_state)                           /***/
/***/ {                                                                    /***/
/***/   #include "lua_api.h"                                               /***/
/***/ }                                                                    /***/
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
  ylib_register(_state);

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
