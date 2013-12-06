#ifndef DATA__BANK_H
#define DATA__BANK_H

#include "../common/map.h"
#include "../common/vector.h"
#include "../common/utility.h"
#include "../render/gl_handle.h"
#include "../log.h"
#include "../save.h"

class CellBlueprint;
class CellMap;
class Databank;
class Filesystem;
class GameStage;
class GlUtil;
class Tileset;

// Finds and loads resources. Template parameter T is the resource type which
// can be associated with string keys. The Dataset owns all resources and they
// will persist for its lifetime. Requesting resources by invalid names returns
// a reference to a standard missing resource.
template<typename T>
class Dataset : public y::no_copy {
public:

  Dataset(T& default_resource);

  // Save specific resource to filesystem.
  void save(Filesystem& filesystem, const Databank& bank,
            const y::string& name, bool human_readable = false) const;

  // Save all resources to filesystem.
  void save_all(Filesystem& filesystem, const Databank& bank,
                bool human_readable = false) const;

  // Add a new resource.
  void insert(const y::string& name, y::unique<T> resource);
  y::size size() const;
  bool empty() const;

  // Get list of loaded resource names.
  const y::vector<y::string>& get_names() const;
  // Check if a name is used.
  bool is_name_used(const y::string& name) const;

  // Get resource by name.
  const T& get(const y::string& name) const;
  /***/ T& get(const y::string& name);

  // Get resource by index.
  const T& get(y::size index) const;
  /***/ T& get(y::size index);

  // Get name resource is stored as.
  const y::string& get_name(const T& resource) const;
  const y::string& get_name(y::size index) const;
  // Get index resource is stored at.
  y::size get_index(const T& resource) const;
  y::size get_index(const y::string& name) const;

  // Rename resource.
  void rename(const T& resource, const y::string& new_name);

  // Clear everything.
  void clear();

private:

  typedef y::map<y::string, y::unique<T>> map_type;
  y::vector<y::string> _list;
  map_type _map;
  T& _default;

};

struct LuaFile {
  y::string path;
  y::string contents;

  // Editor display.
  y::fvec4 yedit_colour;
};

#ifndef SPRITE_DEC
#define SPRITE_DEC
struct Sprite {
  GlTexture2D texture;
  GlTexture2D normal;
};
#endif

class Databank : public y::no_copy {
public:

  ~Databank();
  Databank();
  Databank(const Filesystem& filesystem, GlUtil& gl,
           bool load_yedit_data = false);

  void reload_cells_and_maps(const Filesystem& filesystem);

  // Get the dataset for a generic type.
  template<typename T>
  const Dataset<T>& get_set() const;
  template<typename T>
  /***/ Dataset<T>& get_set();

  // Save resource of a given type.
  template<typename T>
  void save_name(Filesystem& filesystem, const y::string& name) const;
  template<typename T>
  void save(Filesystem& filesystem, const T& t) const;
  // Save all the resources of a certain type.
  template<typename T>
  void save_all(Filesystem& filesystem) const;

private:

  void make_default_map();
  // Load the script and call some functions to get its info.
  void make_lua_file(LuaFile& file, GameStage& gl);

  // Used to delete the textures when the Databank is destroyed.
  y::vector<GlUnique<GlTexture2D>> _textures;

  y::unique<LuaFile> _default_script;
  y::unique<Sprite> _default_sprite;
  y::unique<Tileset> _default_tileset;
  y::unique<CellBlueprint> _default_cell;
  y::unique<CellMap> _default_map;

public:

  Dataset<LuaFile> scripts;
  Dataset<Sprite> sprites;
  Dataset<Tileset> tilesets;
  Dataset<CellBlueprint> cells;
  Dataset<CellMap> maps;

};

template<typename T>
Dataset<T>::Dataset(T& default_resource)
  : _default(default_resource)
{
}

template<typename T>
void Dataset<T>::save(Filesystem& filesystem, const Databank& bank,
                      const y::string& name, bool human_readable) const
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    it->second->y::template io<typename T::io_protobuf_type>::save(
        filesystem, bank, name, human_readable);
  }
}

template<typename T>
void Dataset<T>::save_all(Filesystem& filesystem, const Databank& bank,
                          bool human_readable) const
{
  for (const y::string& name : _list) {
    save(filesystem, bank, name, human_readable);
  }
}

template<typename T>
void Dataset<T>::insert(const y::string& name, y::unique<T> resource)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    it->second.swap(resource);
    return;
  }

  _list.emplace_back(name);
  y::sort(_list.begin(), _list.end());
  auto jt = _map.insert(y::make_pair(name, y::unique<T>()));
  jt.first->second.swap(resource);
}

template<typename T>
y::size Dataset<T>::size() const
{
  return _list.size();
}

template<typename T>
bool Dataset<T>::empty() const
{
  return _list.empty();
}

template<typename T>
const y::vector<y::string>& Dataset<T>::get_names() const
{
  return _list;
}

template<typename T>
bool Dataset<T>::is_name_used(const y::string& name) const
{
  for (const y::string& s : _list) {
    if (s == name) {
      return true;
    }
  }
  return false;
}

template<typename T>
const T& Dataset<T>::get(const y::string& name) const
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  log_err("Couldn't find resource ", name);
  return _default;
}

template<typename T>
T& Dataset<T>::get(const y::string& name)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  log_err("Couldn't find resource ", name);
  return _default;
}

template<typename T>
const T& Dataset<T>::get(y::size index) const
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  log_err("Couldn't find resource ", index);
  return _default;
}

template<typename T>
T& Dataset<T>::get(y::size index)
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  log_err("Couldn't find resource ", index);
  return _default;
}

template<typename T>
const y::string& Dataset<T>::get_name(const T& resource) const
{
  static y::string none = "<no resource>";
  for (const auto& pair : _map) {
    if (pair.second.get() == &resource) {
      return pair.first;
    }
  }
  log_err("Couldn't find name of resource");
  return none;
}

template<typename T>
const y::string& Dataset<T>::get_name(y::size index) const
{
  return get_name(get(index));
}

template<typename T>
y::size Dataset<T>::get_index(const T& resource) const
{
  for (y::size i = 0; i < _list.size(); ++i) {
    const y::string& s = _list[i];
    auto it = _map.find(s);
    if (it != _map.end() && it->second.get() == &resource) {
      return i;
    }
  }
  log_err("Couldn't find index of resource");
  return -1;
}

template<typename T>
y::size Dataset<T>::get_index(const y::string& name) const
{
  return get_index(get(name));
}

template<typename T>
void Dataset<T>::rename(const T& resource, const y::string& new_name)
{
  y::size i = get_index(resource);
  if (i == y::size(-1)) {
    return;
  }
  _list.erase(_list.begin() + i);

  const y::string& name = get_name(resource);
  auto it = _map.find(name);
  if (it == _map.end()) {
    return;
  }
  y::unique<T> temp;
  temp.swap(it->second);
  _map.erase(it);

  _list.emplace_back(new_name);
  y::sort(_list.begin(), _list.end());
  auto jt = _map.insert(y::make_pair(new_name, y::unique<T>()));
  jt.first->second.swap(temp);
}

template<typename T>
void Dataset<T>::clear()
{
  _list.clear();
  _map.clear();
}

template<>
inline const Dataset<LuaFile>& Databank::get_set<LuaFile>() const
{
  return scripts;
}

template<>
inline Dataset<LuaFile>& Databank::get_set<LuaFile>()
{
  return scripts;
}

template<>
inline const Dataset<Sprite>& Databank::get_set<Sprite>() const
{
  return sprites;
}

template<>
inline Dataset<Sprite>& Databank::get_set<Sprite>()
{
  return sprites;
}

template<>
inline const Dataset<Tileset>& Databank::get_set<Tileset>() const
{
  return tilesets;
}

template<>
inline Dataset<Tileset>& Databank::get_set<Tileset>()
{
  return tilesets;
}

template<>
inline const Dataset<CellBlueprint>& Databank::get_set<CellBlueprint>() const
{
  return cells;
}

template<>
inline Dataset<CellBlueprint>& Databank::get_set<CellBlueprint>()
{
  return cells;
}

template<>
inline const Dataset<CellMap>& Databank::get_set<CellMap>() const
{
  return maps;
}

template<>
inline Dataset<CellMap>& Databank::get_set<CellMap>()
{
  return maps;
}

template<typename T>
void Databank::save_name(Filesystem& filesystem, const y::string& name) const
{
  get_set<T>().template save(filesystem, *this, name);
}

template<typename T>
void Databank::save(Filesystem& filesystem, const T& t) const
{
  save_name<T>(filesystem, get_set<T>().get_name(t));
}

template<typename T>
void Databank::save_all(Filesystem& filesystem) const
{
  get_set<T>().template save_all(filesystem, *this);
}

#endif
