#ifndef DATABANK_H
#define DATABANK_H

#include "common.h"
#include "cell.h"
#include "tileset.h"

#include <algorithm>
#include <iostream>

class Databank;
class Filesystem;
class GlUtil;

// Globs and loads resources. Requesting resources by invalid names returns
// a reference to a standard missing resource.
// TODO: exceptions?
template<typename T>
class Dataset : public y::no_copy {
public:

  Dataset(T& default_resource);

  // Save all resources to filesystem.
  template<typename P,
           typename std::enable_if<
               std::is_base_of<y::io<P>, T>::value, bool>::type = 0>
  void save_all(Filesystem& filesystem, const Databank& bank,
                bool human_readable = false) const;

  // Add a new resource.
  void insert(const y::string& name, y::unique<T> resource);
  y::size size() const;
  bool empty() const;

  // Get list of loaded resource names.
  const y::string_vector& get_names() const;
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

private:

  typedef y::map<y::string, y::unique<T>> map_type;
  y::string_vector _list;
  map_type _map;
  T& _default;

};

struct LuaFile {
  y::string path;
  y::string contents;
};

class Databank : public y::no_copy {

  LuaFile _default_script;
  GlTexture _default_sprite;
  Tileset _default_tileset;
  CellBlueprint _default_cell;
  CellMap _default_map;

public:

  Databank(const Filesystem& filesystem, GlUtil& gl);

  // Get the dataset for a generic type.
  template<typename T>
  const Dataset<T>& get_set() const;
  template<typename T>
  /***/ Dataset<T>& get_set();

  // Save all the resources of a certain type. Pass the protobuf type as the
  // second parameter.
  template<typename T, typename P>
  void save_all(Filesystem& filesystem) const;

  Dataset<LuaFile> scripts;
  Dataset<GlTexture> sprites;
  Dataset<Tileset> tilesets;
  Dataset<CellBlueprint> cells;
  Dataset<CellMap> maps;

private:

  void make_default_map();

};

template<typename T>
Dataset<T>::Dataset(T& default_resource)
  : _default(default_resource)
{
}

template<typename T>
template<typename P,
         typename std::enable_if<
             std::is_base_of<y::io<P>, T>::value, bool>::type>
void Dataset<T>::save_all(Filesystem& filesystem, const Databank& bank,
                          bool human_readable) const
{
  for (const y::string& s : _list) {
    auto it = _map.find(s);
    if (it == _map.end()) {
      continue;
    }
    it->second->y::template io<P>::save(filesystem, bank, s, human_readable);
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

  _list.push_back(name);
  std::sort(_list.begin(), _list.end());
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
const y::string_vector& Dataset<T>::get_names() const
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
  std::cerr << "Couldn't find resource " << name << std::endl;
  return _default;
}

template<typename T>
T& Dataset<T>::get(const y::string& name)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  std::cerr << "Couldn't find resource " << name << std::endl;
  return _default;
}

template<typename T>
const T& Dataset<T>::get(y::size index) const
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  std::cerr << "Couldn't find resource " << index << std::endl;
  return _default;
}

template<typename T>
T& Dataset<T>::get(y::size index)
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  std::cerr << "Couldn't find resource " << index << std::endl;
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
  std::cerr << "Could find name of resource" << std::endl;
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
  std::cerr << "Could find index of resource" << std::endl;
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

  _list.push_back(new_name);
  std::sort(_list.begin(), _list.end());
  auto jt = _map.insert(y::make_pair(new_name, y::unique<T>()));
  jt.first->second.swap(temp);
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
inline const Dataset<GlTexture>& Databank::get_set<GlTexture>() const
{
  return sprites;
}

template<>
inline Dataset<GlTexture>& Databank::get_set<GlTexture>()
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

template<typename T, typename P>
void Databank::save_all(Filesystem& filesystem) const
{
  get_set<T>().template save_all<P>(filesystem, *this);
}
#endif
