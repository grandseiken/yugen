#ifndef LUA_H
#define LUA_H

#include "common.h"
#include "vector.h"
#include "lua_types.h"

class Collision;
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
  ~ScriptReference();

  ScriptReference& operator=(const ScriptReference& arg);
  bool operator==(const ScriptReference& arg) const;
  bool operator!=(const ScriptReference& arg) const;

  bool is_valid() const;
  void invalidate();

  const Script* get() const;
  /***/ Script* get();

  const Script& operator*() const;
  /***/ Script& operator*();

  const Script* operator->() const;
  /***/ Script* operator->();

private:

  Script* _script;

};

// And the const version.
class ConstScriptReference {
public:

  ConstScriptReference(const Script& script);
  ConstScriptReference(const ScriptReference& script);
  ConstScriptReference(const ConstScriptReference& script);
  ~ConstScriptReference();

  ConstScriptReference& operator=(const ConstScriptReference& arg);
  bool operator==(const ConstScriptReference& arg) const;
  bool operator!=(const ConstScriptReference& arg) const;

  bool is_valid() const;
  void invalidate();

  const Script* get() const;
  const Script& operator*() const;
  const Script* operator->() const;

private:

  const Script* _script;

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
  // We require the Collision when changing in order to update the spatial hash.
  void set_origin(const y::wvec2& origin, const Collision& collision);
  void set_rotation(y::world rotation, const Collision& collision);

  typedef y::vector<LuaValue> lua_args;

  bool has_function(const y::string& function_name) const;
  void call(const y::string& function_name, const lua_args& args = {});
  void call(lua_args& output, const y::string& function_name,
            const lua_args& args = {});

  void destroy();
  bool is_destroyed() const;

private:

  y::string _path;
  lua_State* _state;

  y::wvec2 _origin;
  y::wvec2 _region;
  y::world _rotation;

  bool _destroyed;

  friend class ScriptReference;
  friend class ConstScriptReference;
  typedef y::set<ScriptReference*> reference_set;
  typedef y::set<ConstScriptReference*> const_reference_set;
  reference_set _reference_set;
  mutable const_reference_set _const_reference_set;

};

// Stores objects associated with Scripts which are destroyed when the Script
// is destroyed.
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
  void clean_up();

  const entry_list& get_list(const Script& source) const;
  void get_sources(source_list& output) const;

  // Override for custom behaviour extension.
  virtual void on_create(const Script& source, T* obj);
  virtual void on_destroy(const Script& source, T* obj);

private:

  // We hold a weak reference to each Script so that we can destroy the objects
  // whose source Scripts no longer exist.
  struct map_entry {
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
    it = _map.insert(y::make_pair(&source, map_entry{
             ConstScriptReference(source), y::vector<entry>()})).first;
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
  on_destroy(source, obj);
}

template<typename T>
void ScriptMap<T>::destroy_all(const Script& source)
{
  auto it = _map.find(const_cast<Script*>(&source));
  if (it != _map.end()) {
    for (auto jt = it->second.list.begin(); jt != it->second.list.end(); ++jt) {
      on_destroy(source, jt->get());
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
        on_destroy(*it->first, jt->get());
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
void ScriptMap<T>::on_create(const Script& source, T* obj)
{
  (void)source;
  (void)obj;
}

template<typename T>
void ScriptMap<T>::on_destroy(const Script& source, T* obj)
{
  (void)source;
  (void)obj;
}

template<typename T>
const y::vector<typename ScriptMap<T>::entry> ScriptMap<T>::no_list;

#endif
