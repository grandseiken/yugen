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
  inline T check(lua_State* state, ylib_int index);

  // Integral values.
  template<>
  inline ylib_int check<ylib_int>(lua_State* state, ylib_int index)
  {
    return luaL_checkinteger(state, index);
  }

  inline void push(lua_State* state, ylib_int arg)
  {
    lua_pushinteger(state, arg);
  }

  // Floating-point values.
  template<>
  inline float check<float>(lua_State* state, ylib_int index)
  {
    return luaL_checknumber(state, index);
  }

  inline void push(lua_State* state, float arg)
  {
    lua_pushnumber(state, arg);
  }

  // Boolean values.
  template<>
  inline bool check<bool>(lua_State* state, ylib_int index)
  {
    return lua_toboolean(state, index);
  }

  inline void push(lua_State* state, bool arg)
  {
    lua_pushboolean(state, arg);
  }

  // String values.
  template<>
  inline y::string check<y::string>(lua_State* state, ylib_int index)
  {
    return y::string(luaL_checkstring(state, index));
  }

  inline void push(lua_State* state, const y::string& arg)
  {
    lua_pushstring(state, arg.c_str());
  }

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

/******************************************************************************/
/**** Preprocessor magic for defining functions callable by Lua in a style ****/
/**** that sort of approximates C functions:                               ****/
/******************************************************************************/
/***/ #define ylib_api(name)                                               /**/\
/***/ namespace _ylib_api_##name {                                         /**/\
/***/   ylib::ylib_int _ylib_api(lua_State* _ylib_state);                  /**/\
/***/   typedef ylib::ylib_int _ylib_api_type(lua_State* _ylib_state);     /**/\
/***/   static const char* const _ylib_name = #name;                       /**/\
/***/   static _ylib_api_type* const _ylib_ref = _ylib_api;                /**/\
/***/   ylib::ylib_int _ylib_api(lua_State* _ylib_state)                   /**/\
/***/   {                                                                  /**/\
/***/     ylib::ylib_int _ylib_stack = 0;                                  /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/     T name = ylib::check<T>(_ylib_state, ++_ylib_stack);             /***/
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
/**** storing them in a Lua stack:                                         ****/
/******************************************************************************/
/***/ #undef ylib_api                                                      /***/
/***/ #undef ylib_arg                                                      /***/
/***/ #undef ylib_return                                                   /***/
/***/ #undef ylib_void                                                     /***/
/***/ #define ylib_api(name)                                               /**/\
/***/   lua_register(_YLIB_LUA_STATE,                                      /**/\
/***/                _ylib_api_##name::_ylib_name,                         /**/\
/***/                _ylib_api_##name::_ylib_ref);                         /**/\
/***/   struct _ylib_no_op_struct_##name {                                 /**/\
/***/     static void no_op()                                              /**/\
/***/     {                                                                /***/
/***/ #define ylib_arg(T, name)                                            /**/\
/***/       T name = ylib::check<T>(y::null, 0);                           /**/\
/***/       (void)name;                                                    /***/
/***/ #define ylib_return(...)                                             /**/\
/***/       }                                                              /**/\
/***/     }                                                                /**/\
/***/   }; while (false) {(void)0                                          /***/
/***/ #define ylib_void() ylib_return()                                    /***/
/***/ void ylib_register(lua_State* _YLIB_LUA_STATE)                       /***/
/***/ {                                                                    /***/
/***/   #include "lua_api.h"                                               /***/
/***/ }                                                                    /***/
/******************************************************************************/
/**** End of proprocessor magic.                                           ****/
/******************************************************************************/

// TODO: figure out code-sharing between scripts. (Make some master lua code
// in a file and chuck it into the global state of each new script?)
Script::Script(const y::string& path, const y::string& contents)
  : _path(path)
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
