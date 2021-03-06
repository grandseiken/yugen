#ifndef GAME_SAVEGAME_H
#define GAME_SAVEGAME_H

#include "../lua_types.h"
#include "../save.h"
#include <string>
#include <unordered_map>

class Filesystem;
namespace proto {
  class Value;
  class Savegame;
}

// Savegames are slightly different to the usual serialisable objects we deal
// with, in that we may want to save and load them arbitrarily during one
// execution of the game (rather than simply load everything to immutable
// objects once at the start and never care again outside of the editor).
//
// For this reason, the Savegame should not need to save or load any dependent
// objects, so we provide convenience functions to save and load without a
// Databank.
class Savegame : public y::io<proto::Savegame> {
public:

  Savegame();
  void save(Filesystem& filesystem,
            const std::string& path, bool human_readable = false) const;
  void load(const Filesystem& filesystem,
            const std::string& path, bool human_readable = false);

  void clear();
  const LuaValue& get(const std::string& key) const;
  void put(const std::string& key, const LuaValue& value);

protected:

  void save_to_proto(
      const Databank& bank, proto::Savegame& proto) const override;
  void load_from_proto(
      Databank& bank, const proto::Savegame& proto) override;

private:

  typedef std::unordered_map<std::string, LuaValue> value_map;
  value_map _map;
  LuaValue _default_value;

};

#endif
