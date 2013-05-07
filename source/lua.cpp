#include "lua.h"
#include "filesystem.h"

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

  // Integral values.
  template<>
  struct check<ylib_int> {
    inline ylib_int operator()(lua_State* state, ylib_int index) const
    {
      return luaL_checkinteger(state, index);
    }
  };

  inline void push(lua_State* state, ylib_int arg)
  {
    lua_pushinteger(state, arg);
  }

  // Floating-point values.
  template<>
  struct check<float> {
    inline float operator()(lua_State* state, ylib_int index) const
    {
      return luaL_checknumber(state, index);
    }
  };

  inline void push(lua_State* state, float arg)
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
      return y::string(luaL_checkstring(state, index));
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
      luaL_checktype(state, index, LUA_TTABLE);

      ylib_int size = luaL_len(state, index);
      for (ylib_int i = 1; i <= size; ++i) {
        lua_rawgeti(state, index, i);
        t.push_back(check<T>()(state, lua_gettop(state)));
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
      void* v = luaL_checkudata(state, index, type_name<T>::name);
      T** t = reinterpret_cast<T**>(v);
      return *t;
    }
  };

  template<typename T>
  inline void push(lua_State* state, T* arg)
  {
    T** t = reinterpret_cast<T**>(lua_newuserdata(state, sizeof(T*)));
    luaL_getmetatable(state, type_name<T>::name);
    lua_setmetatable(state, -2);
    *t = arg;
  }

  // Generic pointer values. These can only be passed from Lua to C++, not
  // the other way round.
  template<>
  struct check<void*> {
    inline void* operator()(lua_State* state, ylib_int index) const
    {
      void* v = lua_touserdata(state, index);
      bool b = lua_getmetatable(state, index);
      lua_pop(state, 1);
      luaL_argcheck(state, v && b, index, "ylib.Void expected");
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
static RegistryIndex script_registry_index;

/******************************************************************************/
/**** Preprocessor magic for defining types and functions useable by Lua   ****/
/**** in a style that sort of approximates C functions:                    ****/
/******************************************************************************/
/***/ #define ylib_typedef(T)                                              /**/\
/***/ namespace ylib {                                                     /**/\
/***/   template<>                                                         /**/\
/***/   struct type_name<T> {                                              /**/\
/***/     static const char* const name;                                   /**/\
/***/   };                                                                 /**/\
/***/   const char* const type_name<T>::name = "ylib." #T;                 /**/\
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
/***/         reinterpret_cast<void*>(&script_registry_index));            /**/\
/***/     lua_gettable(_ylib_state, LUA_REGISTRYINDEX);                    /**/\
/***/     Script* self = *reinterpret_cast<Script**>(                      /**/\
/***/         lua_touserdata(_ylib_state, -1));                            /**/\
/***/     lua_pop(_ylib_state, 1);                                         /**/\
/***/     GameStage& stage = self->get_stage();                            /**/\
/***/     (void)self;                                                      /**/\
/***/     (void)stage;                                                     /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/     T name = ylib::check<T>()(_ylib_state, ++_ylib_stack);           /***/
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
ylib_typedef(Script);
#include "lua_api.h"
/******************************************************************************/
/**** Preprocessor magic for enumerating the library of functions and      ****/
/**** types and storing them in a Lua stack:                               ****/
/******************************************************************************/
/***/ #undef ylib_typedef                                                  /***/
/***/ #undef ylib_api                                                      /***/
/***/ #undef ylib_arg                                                      /***/
/***/ #undef ylib_checked_arg                                              /***/
/***/ #undef ylib_return                                                   /***/
/***/ #undef ylib_void                                                     /***/
/***/ #define ylib_typedef(T)                                              /**/\
/***/   luaL_newmetatable(_ylib_state, ylib::type_name<T>::name);          /**/\
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
/***/       Script* self = y::null;                                        /**/\
/***/       GameStage& stage = self->get_stage();                          /**/\
/***/       (void)self;                                                    /**/\
/***/       (void)stage;                                                   /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/       T name = ylib::check<T>()(y::null, 0);                         /**/\
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

Script::Script(GameStage& stage,
               const y::string& path, const y::string& contents)
  : _stage(stage)
  , _path(path)
  // Use standard allocator and panic function.
  , _state(luaL_newstate())
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

  // Set self-reference in the registry.
  lua_pushlightuserdata(
      _state, reinterpret_cast<void*>(&script_registry_index));
  ylib::push(_state, this);
  lua_settable(_state, LUA_REGISTRYINDEX);

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
}

const GameStage& Script::get_stage() const
{
  return _stage;
}

GameStage& Script::get_stage()
{
  return _stage;
}

const y::string& Script::get_path() const
{
  return _path;
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
