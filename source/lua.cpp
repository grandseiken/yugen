#include "lua.h"

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

// Template / overload functions for interopability of different types
// with Lua.
namespace ylib {

  typedef y::int32 ylib_int;

  template<typename T>
  struct check {};
  template<typename T>
  struct type_name {};
  template<>
  struct type_name<y::int32> {
    static const y::string name;
  };
  const y::string type_name<y::int32>::name = "int";
  template<>
  struct type_name<y::world> {
    static const y::string name;
  };
  const y::string type_name<y::world>::name = "world";
  template<>
  struct type_name<bool> {
    static const y::string name;
  };
  const y::string type_name<bool>::name = "bool";
  template<>
  struct type_name<y::string> {
    static const y::string name;
  };
  const y::string type_name<y::string>::name = "string";
  template<typename T>
  struct type_name<y::vector<T>> {
    static const y::string name;
  };
  template<typename T>
  const y::string type_name<y::vector<T>>::name =
      "vector<" + type_name<T>::name + ">";
  template<typename T, y::size N>
  struct type_name<y::vec<T, N>> {
    static const y::string name;
  };
  template<typename T, y::size N>
  const y::string type_name<y::vec<T, N>>::name =
      "vec<" + type_name<T>::name + ", " + y::to_string(N) + ">";

  // Integral values.
  template<>
  struct check<ylib_int> {
    inline ylib_int operator()(lua_State* state, ylib_int index) const
    {
#ifndef DEBUG
      return lua_tointeger(state, index);
#else
      return luaL_checkinteger(state, index);
#endif
    }
  };

  inline void push(lua_State* state, ylib_int arg)
  {
    lua_pushinteger(state, arg);
  }

  // Floating-point values.
  template<>
  struct check<double> {
    inline double operator()(lua_State* state, ylib_int index) const
    {
#ifndef DEBUG
      return lua_tonumber(state, index);
#else
      return luaL_checknumber(state, index);
#endif
    }
  };

  inline void push(lua_State* state, double arg)
  {
    lua_pushnumber(state, arg);
  }

  // Boolean values.
  template<>
  struct check<bool> {
    inline bool operator()(lua_State* state, ylib_int index) const
    {
      return lua_toboolean(state, index);
    }
  };

  inline void push(lua_State* state, bool arg)
  {
    lua_pushboolean(state, arg);
  }

  // String values.
  template<>
  struct check<y::string> {
    inline y::string operator()(lua_State* state, ylib_int index) const
    {
#ifndef DEBUG
      return y::string(lua_tostring(state, index));
#else
      return y::string(luaL_checkstring(state, index));
#endif
    }
  };

  inline void push(lua_State* state, const y::string& arg)
  {
    lua_pushstring(state, arg.c_str());
  }

  // Array values.
  template<typename T>
  struct check<y::vector<T>> {
    inline y::vector<T> operator()(lua_State* state, ylib_int index) const
    {
      y::vector<T> t;
#ifdef DEBUG
      luaL_checktype(state, index, LUA_TTABLE);
#endif

      ylib_int size = luaL_len(state, index);
      for (ylib_int i = 1; i <= size; ++i) {
        lua_rawgeti(state, index, i);
        t.emplace_back(check<T>()(state, lua_gettop(state)));
      }
      lua_pop(state, size);
      return t;
    }
  };

  template<typename T>
  inline void push(lua_State* state, const y::vector<T>& arg)
  {
    lua_createtable(state, arg.size(), 0);
    ylib_int index = lua_gettop(state);
    for (y::size i = 0; i < arg.size(); ++i) {
      push(state, arg[i]);
      lua_rawseti(state, index, 1 + i);
    }
  }

  // Pointer values.
  template<typename T>
  struct check<T*> {
    inline T* operator()(lua_State* state, ylib_int index) const
    {
#ifndef DEBUG
      void* v = lua_touserdata(state, index);
#else
      void* v = luaL_checkudata(state, index, type_name<T>::name.c_str());
#endif
      T** t = reinterpret_cast<T**>(v);
      return *t;
    }
  };

  template<typename T>
  inline void push(lua_State* state, T* arg)
  {
    T** t = reinterpret_cast<T**>(lua_newuserdata(state, sizeof(T*)));
    luaL_getmetatable(state, type_name<T>::name.c_str());
    lua_setmetatable(state, -2);
    *t = arg;
  }

  // Vector values.
  template<typename T, y::size N>
  struct check<const y::vec<T, N>> {
    inline const y::vec<T, N>& operator()(lua_State* state, ylib_int index)
        const
    {
#ifndef DEBUG
      void* v = lua_touserdata(state, index);
#else
      void* v = luaL_checkudata(state, index,
                                type_name<y::vec<T, N>>::name.c_str());
#endif
      y::vec<T, N>* t = reinterpret_cast<y::vec<T, N>*>(v);
      return *t;
    }
  };

  template<typename T, y::size N>
  struct check<y::vec<T, N>> {
    inline y::vec<T, N>& operator()(lua_State* state, ylib_int index) const
    {
#ifndef DEBUG
      void* v = lua_touserdata(state, index);
#else
      void* v = luaL_checkudata(state, index,
                                type_name<y::vec<T, N>>::name.c_str());
#endif
      y::vec<T, N>* t = reinterpret_cast<y::vec<T, N>*>(v);
      return *t;
    }
  };

  template<typename T, y::size N>
  inline void push(lua_State* state, const y::vec<T, N>& arg)
  {
    y::vec<T, N>* t = reinterpret_cast<y::vec<T, N>*>(
        lua_newuserdata(state, sizeof(y::vec<T, N>)));
    static const y::string name = type_name<y::vec<T, N>>::name;
    luaL_getmetatable(state, name.c_str());
    lua_setmetatable(state, -2);
    *t = arg;
  }

  // Script weak references.
  template<>
  struct check<const ScriptReference> {
    inline const ScriptReference& operator()(lua_State* state, ylib_int index)
        const
    {
#ifndef DEBUG
      void* v = lua_touserdata(state, index);
#else
      void* v = luaL_checkudata(state, index, "ylib.ScriptReference");
#endif
      ScriptReference* t = reinterpret_cast<ScriptReference*>(v);
      return *t;
    }
  };

  template<>
  struct check<ScriptReference> {
    inline ScriptReference& operator()(lua_State* state, ylib_int index)
    {
#ifndef DEBUG
      void* v = lua_touserdata(state, index);
#else
      void* v = luaL_checkudata(state, index, "ylib.ScriptReference");
#endif
      ScriptReference* t = reinterpret_cast<ScriptReference*>(v);
      return *t;
    }
  };

  inline void push(lua_State* state, const ScriptReference& arg)
  {
    ScriptReference* t = reinterpret_cast<ScriptReference*>(
        lua_newuserdata(state, sizeof(ScriptReference)));
    luaL_getmetatable(state, "ylib.ScriptReference");
    lua_setmetatable(state, -2);
    new (t) ScriptReference(arg);
  }

  // Generic pointer values. These can only be passed from Lua to C++, not
  // the other way round.
  template<>
  struct check<void*> {
    inline void* operator()(lua_State* state, ylib_int index) const
    {
      void* v = lua_touserdata(state, index);
#ifdef DEBUG
      bool b = lua_getmetatable(state, index);
      lua_pop(state, 1);
      luaL_argcheck(state, v && b, index, "ylib.Void expected");
#endif
      void** t = reinterpret_cast<void**>(v);
      return *t;
    }
  };

  // Variadic push.
  inline ylib_int push_all(lua_State* state)
  {
    (void)state;
    return 0;
  }

  template<typename T, typename... U>
  inline ylib_int push_all(lua_State* state, const T& arg, const U&... args)
  {
    push(state, arg);
    push_all(state, args...);
    return 1 + sizeof...(U);
  }

}

struct RegistryIndex {};
static RegistryIndex stage_registry_index;

/******************************************************************************/
/**** Preprocessor magic for defining types and functions useable by Lua   ****/
/**** in a style that sort of approximates C functions:                    ****/
/******************************************************************************/
/***/ #define ylib_typedef(T)                                              /**/\
/***/ namespace ylib {                                                     /**/\
/***/   template<>                                                         /**/\
/***/   struct type_name<T> {                                              /**/\
/***/     static const y::string name;                                     /**/\
/***/   };                                                                 /**/\
/***/   template<>                                                         /**/\
/***/   struct type_name<const T> {                                        /**/\
/***/     static const y::string name;                                     /**/\
/***/   };                                                                 /**/\
/***/   const y::string type_name<T>::name = "ylib." #T;                   /**/\
/***/   const y::string type_name<const T>::name = "ylib." #T;             /**/\
/***/   ylib_api(T##_eq) ylib_arg(T*, a) ylib_arg(T*, b)                   /**/\
/***/   {                                                                  /**/\
/***/     ylib_return(a == b);                                             /**/\
/***/   }                                                                  /**/\
/***/ } void _ylib_typedef_##name_no_op()                                  /***/
/***/ #define ylib_api(name)                                               /**/\
/***/ namespace _ylib_api_##name {                                         /**/\
/***/   ylib::ylib_int _ylib_api(lua_State* _ylib_state);                  /**/\
/***/   typedef ylib::ylib_int _ylib_api_type(lua_State* _ylib_state);     /**/\
/***/   static const char* const _ylib_name = #name;                       /**/\
/***/   static _ylib_api_type* const _ylib_ref = _ylib_api;                /**/\
/***/   ylib::ylib_int _ylib_api(lua_State* _ylib_state)                   /**/\
/***/   {                                                                  /**/\
/***/     ylib::ylib_int _ylib_stack = 0;                                  /**/\
/***/     (void)_ylib_stack;                                               /**/\
/***/     lua_pushlightuserdata(                                           /**/\
/***/         _ylib_state,                                                 /**/\
/***/         reinterpret_cast<void*>(&stage_registry_index));             /**/\
/***/     lua_gettable(_ylib_state, LUA_REGISTRYINDEX);                    /**/\
/***/     GameStage& stage = *ylib::check<GameStage*>()(                   /**/\
/***/         _ylib_state, -1);                                            /**/\
/***/     lua_pop(_ylib_state, 1);                                         /**/\
/***/     (void)stage;                                                     /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/     T name = ylib::check<T>()(_ylib_state, ++_ylib_stack);           /***/
/***/ #define ylib_refarg(T, name)                                         /**/\
/***/     T& name = ylib::check<T>()(_ylib_state, ++_ylib_stack);          /***/
/***/ #define ylib_checked_arg(T, name, assert, message)                   /**/\
/***/     ylib_arg(T, name)                                                /**/\
/***/     luaL_argcheck(_ylib_state, (assert), _ylib_stack, (message));    /***/
/***/ #define ylib_return(...)                                             /**/\
/***/       return ylib::push_all(_ylib_state, __VA_ARGS__);               /**/\
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
/***/ #undef ylib_typedef                                                  /***/
/***/ #undef ylib_api                                                      /***/
/***/ #undef ylib_arg                                                      /***/
/***/ #undef ylib_refarg                                                   /***/
/***/ #undef ylib_checked_arg                                              /***/
/***/ #undef ylib_return                                                   /***/
/***/ #undef ylib_void                                                     /***/
/***/ #define ylib_typedef(T)                                              /**/\
/***/   luaL_newmetatable(_ylib_state, ylib::type_name<T>::name.c_str());  /**/\
/***/   lua_pushstring(_ylib_state, "__eq");                               /**/\
/***/   lua_pushcfunction(_ylib_state,                                     /**/\
/***/                     ylib::_ylib_api_##T##_eq::_ylib_ref);            /**/\
/***/   lua_settable(_ylib_state, -3);                                     /**/\
/***/   lua_pop(_ylib_state, 1)                                            /***/
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
/***/       T name = ylib::check<T>()(y::null, 0);                         /**/\
/***/       (void)name;                                                    /***/
/***/ #define ylib_refarg(T, name)                                         /**/\
/***/       T& name = ylib::check<T>()(y::null, 0);                        /**/\
/***/       (void)name;                                                    /***/
/***/ #define ylib_checked_arg(T, name, assert, message)                   /**/\
/***/       ylib_arg(T, name)                                              /***/
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

#define ylib_new_metatable(name)\
  luaL_newmetatable(_ylib_state, name);\
  lua_pushstring(_ylib_state, "__index");\
  lua_pushvalue(_ylib_state, -2);\
  lua_settable(_ylib_state, -3);
#define ylib_add_method(name, function)\
  lua_pushstring(_ylib_state, name);\
  lua_pushcfunction(_ylib_state,\
                    _ylib_api_##function::_ylib_ref);\
  lua_settable(_ylib_state, -3);

template<typename V>
void ylib_register_vectors(lua_State* _ylib_state)
{
  ylib_new_metatable(ylib::type_name<V>::name.c_str());
  ylib_add_method("__add", vec_add);
  ylib_add_method("__sub", vec_sub);
  ylib_add_method("__mul", vec_mul);
  ylib_add_method("__div", vec_div);
  ylib_add_method("__mod", vec_mod);
  ylib_add_method("__unm", vec_unm);
  ylib_add_method("__eq", vec_eq);
  ylib_add_method("x", vec_x);
  ylib_add_method("y", vec_y);
  ylib_add_method("normalised", vec_normalised);
  ylib_add_method("normalise", vec_normalise);
  ylib_add_method("dot", vec_dot);
  ylib_add_method("length_squared", vec_length_squared);
  ylib_add_method("length", vec_length);
  ylib_add_method("angle", vec_angle);
  ylib_add_method("in_region", vec_in_region);
  ylib_add_method("abs", vec_abs);
  ylib_add_method("max", vec_max);
  ylib_add_method("min", vec_min);
  lua_pop(_ylib_state, 1);
}

void ylib_register_references(lua_State* _ylib_state)
{
  ylib_new_metatable("ylib.ScriptReference");
  ylib_add_method("__eq", ref_eq);
  ylib_add_method("__gc", ref_gc);
  ylib_add_method("valid", ref_valid);
  ylib_add_method("get", ref_get);
  lua_pop(_ylib_state, 1);
}

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
  , _destroyed(false)
{
  // Load the Lua standard library.
  luaL_openlibs(_state);
  // Register Lua API.
  ylib_register(_state);
  ylib_register_vectors<y::wvec2>(_state);
  ylib_register_references(_state);

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
  ylib::push(_state, &stage);
  lua_settable(_state, LUA_REGISTRYINDEX);

  // Set self-reference global variable.
  ylib::push(_state, this);
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

void Script::set_origin(const y::wvec2& origin)
{
  _origin = origin;
}

void Script::set_region(const y::wvec2& region)
{
  _region = region;
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

void Script::call(const y::string& function_name, y::int32 arg)
{
  lua_getglobal(_state, function_name.c_str());
  ylib::push(_state, arg);
  if (lua_pcall(_state, 1, 0, 1)) {
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
