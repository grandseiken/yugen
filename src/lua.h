#ifndef LUA_H
#define LUA_H

#include "common/map.h"
#include "common/set.h"
#include "callback.h"
#include "vec.h"
#include "lua_types.h"

class Filesystem;
class GameStage;
class Script;
struct lua_State;

// Weak reference to a Script. Invalidated automatically if the Script is
// destroyed.
class ScriptReference {
public:

  ScriptReference(Script& script);
  ScriptReference(const ScriptReference& script);
  ScriptReference(ScriptReference&& script) = delete;
  ~ScriptReference();

  ScriptReference& operator=(const ScriptReference& arg);
  ScriptReference& operator=(ScriptReference&& arg) = delete;
  bool operator==(const ScriptReference& arg) const;
  bool operator!=(const ScriptReference& arg) const;

  bool is_valid() const;
  void invalidate(Script* script);

  const Script* get() const;
  /***/ Script* get();

  const Script& operator*() const;
  /***/ Script& operator*();

  const Script* operator->() const;
  /***/ Script* operator->();

private:

  Script* _script;
  y::int32 _callback_id;

};

// And the const version.
class ConstScriptReference {
public:

  ConstScriptReference(const Script& script);
  ConstScriptReference(const ScriptReference& script);
  ConstScriptReference(const ConstScriptReference& script);
  ConstScriptReference(ConstScriptReference&& script) = delete;
  ~ConstScriptReference();

  ConstScriptReference& operator=(const ConstScriptReference& arg);
  ConstScriptReference& operator=(ConstScriptReference&& arg) = delete;
  bool operator==(const ConstScriptReference& arg) const;
  bool operator!=(const ConstScriptReference& arg) const;

  bool is_valid() const;
  void invalidate(Script* script);

  const Script* get() const;
  const Script& operator*() const;
  const Script* operator->() const;

private:

  const Script* _script;
  y::int32 _callback_id;

};

class Script : public y::no_copy {
public:

  Script(GameStage& stage, const y::string& path, const y::string& contents,
         const y::wvec2& origin, const y::wvec2& region);
  ~Script();

  const y::string& get_path() const;

  // A Script's region is initially provided on load from the CellMap (or when
  // dynamically created). The origin is always centered in the region. Scripts
  // are created when their region overlaps the WorldWindow, and automatically
  // destroyed when their current region is completely outside it.
  const y::wvec2& get_region() const;
  const y::wvec2& get_origin() const;
  y::world get_rotation() const;
  void set_region(const y::wvec2& region);
  void set_origin(const y::wvec2& origin);
  void set_rotation(y::world rotation);

  typedef y::vector<LuaValue> lua_args;

  bool has_function(const y::string& function_name) const;
  void call(const y::string& function_name, const lua_args& args = {});
  void call(lua_args& output, const y::string& function_name,
            const lua_args& args = {});

  typedef CallbackSet<Script*>::callback callback;
  y::int32 add_move_callback(const callback& callback) const;
  void remove_move_callback(y::int32 id) const;
  y::int32 add_destroy_callback(const callback& callback) const;
  void remove_destroy_callback(y::int32 id) const;

  void destroy();
  bool is_destroyed() const;

private:

  y::string _path;
  lua_State* _state;

  y::wvec2 _origin;
  y::wvec2 _region;
  y::world _rotation;
  bool _destroyed;

  mutable CallbackSet<Script*> _move_callbacks;
  mutable CallbackSet<Script*> _destroy_callbacks;

};

// Associated objects with Scripts which are destroyed when the Script that
// aossicated them is destroyed. T is the associated object type.
template<typename T>
class ScriptMap : public y::no_copy {
public:

  typedef y::unique<T> entry;
  typedef y::vector<entry> entry_list;
  typedef y::vector<const Script*> source_list;

  virtual ~ScriptMap() {}

  T* create_obj(Script& source);
  void destroy_obj(const Script& source, T* obj);
  void destroy_all(const Script& source);
  // Cleans up and destroys the objects whose associated Scripts have been
  // destroyed. Must be called periodically.
  void clean_up();

  const entry_list& get_list(const Script& source) const;
  void get_sources(source_list& output) const;

protected:

  // Override for custom behaviour extension.
  virtual void on_create(const Script& source, T* obj);
  virtual void on_destroy(T* obj);

private:

  // We hold a weak reference to each Script so that we can destroy the objects
  // whose source Scripts no longer exist.
  struct map_entry {
    map_entry(const Script& script)
      : ref(script)
    {}

    ConstScriptReference ref;
    y::vector<entry> list;
  };
  typedef y::map<Script*, map_entry> map;
  map _map;

  static const y::vector<entry> no_list;

};

template<typename T>
T* ScriptMap<T>::create_obj(Script& source)
{
  T* obj = new T(source);
  auto it = _map.find(&source);
  if (it != _map.end() && !it->second.ref.is_valid()) {
    _map.erase(it);
    it = _map.end();
  }
  if (it == _map.end()) {
    it = _map.emplace(&source, source).first;
  }
  it->second.list.emplace_back(obj);
  on_create(source, obj);
  return obj;
}

template<typename T>
void ScriptMap<T>::destroy_obj(const Script& source, T* obj)
{
  auto it = _map.find(const_cast<Script*>(&source));
  if (it != _map.end()) {
    for (auto jt = it->second.list.begin(); jt != it->second.list.end(); ++jt) {
      if (jt->get() == obj) {
        it->second.list.erase(jt);
        break;
      }
    }
    if (it->second.list.empty() || !it->second.ref.is_valid()) {
      _map.erase(it);
    }
  }
  on_destroy(obj);
}

template<typename T>
void ScriptMap<T>::destroy_all(const Script& source)
{
  auto it = _map.find(const_cast<Script*>(&source));
  if (it != _map.end()) {
    for (auto jt = it->second.list.begin(); jt != it->second.list.end(); ++jt) {
      on_destroy(jt->get());
    }
    _map.erase(it);
  }
}

template<typename T>
void ScriptMap<T>::clean_up()
{
  for (auto it = _map.begin(); it != _map.end();) {
    if (it->second.ref.is_valid()) {
      ++it;
    }
    else {
      for (auto jt = it->second.list.begin();
           jt != it->second.list.end(); ++jt) {
        on_destroy(jt->get());
      }
      it = _map.erase(it);
    }
  }
}

template<typename T>
const typename ScriptMap<T>::entry_list& ScriptMap<T>::get_list(
    const Script& source) const
{
  auto it = _map.find(const_cast<Script*>(&source));
  if (it == _map.end() || !it->second.ref.is_valid()) {
    return no_list;
  }
  return it->second.list;
}

template<typename T>
void ScriptMap<T>::get_sources(source_list& output) const
{
  for (const auto& pair : _map) {
    if (pair.second.ref.is_valid()) {
      output.emplace_back(pair.first);
    }
  }
}

template<typename T>
void ScriptMap<T>::on_create(const Script&, T*)
{
}

template<typename T>
void ScriptMap<T>::on_destroy(T*)
{
}

template<typename T>
const y::vector<typename ScriptMap<T>::entry> ScriptMap<T>::no_list;

#endif
