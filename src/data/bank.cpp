#include "bank.h"

#include "cell.h"
#include "tileset.h"

#include "../filesystem/filesystem.h"
#include "../filesystem/physical.h"
#include "../game/stage.h"
#include "../render/gl_util.h"
#include "../render/util.h"
#include "../lua.h"
#include "../../gen/proto/cell.pb.h"

Databank::~Databank()
{
}

Databank::Databank()
  : _default_script(
      new LuaFile{"/yedit/missing.lua", "", y::fvec4{1.f, 1.f, 1.f, 1.f}})
  , _default_sprite(new Sprite())
  , _default_tileset(new Tileset(*_default_sprite))
  , _default_cell(new CellBlueprint())
  , _default_map(new CellMap())
  , scripts(*_default_script)
  , sprites(*_default_sprite)
  , tilesets(*_default_tileset)
  , cells(*_default_cell)
  , maps(*_default_map)
  , sounds(_default_sound)
  , musics(_default_music)
{
  log_debug("Created default Databank");
}

Databank::Databank(const Filesystem& filesystem, GlUtil& gl,
                   bool load_yedit_data)
  : _default_script(
      new LuaFile{"/yedit/missing.lua", "", y::fvec4{1.f, 1.f, 1.f, 1.f}})
  , _default_sprite(new Sprite{gl.make_texture("/yedit/missing.png"),
                               gl.make_texture("/default_normal.png")})
  , _default_tileset(new Tileset(*_default_sprite))
  , _default_cell(new CellBlueprint())
  , _default_map(new CellMap())
  , scripts(*_default_script)
  , sprites(*_default_sprite)
  , tilesets(*_default_tileset)
  , cells(*_default_cell)
  , maps(*_default_map)
  , sounds(_default_sound)
  , musics(_default_music)
{
  // Things should be loaded in order of dependence, so that the data can be
  // accessed while loading if necessary. For example, maps depend on cells
  // and scripts.
  std::vector<std::string> paths;
  filesystem.list_pattern(paths, "/sprites/**.png");
  for (const std::string& s : paths) {
    std::string bare_path;
    filesystem.barename(bare_path, s);
    if (bare_path.substr(bare_path.length() - 7) == "_normal") {
      continue;
    }
    log_debug("Loading ", bare_path);
    std::string normal_path = bare_path + "_normal.png";

    GlTexture2D texture = gl.make_texture(s);
    GlTexture2D normal_texture = filesystem.is_file(normal_path) ?
        gl.make_texture(normal_path) : _default_sprite->normal;

    _textures.emplace_back(gl.make_unique(texture));
    if (filesystem.is_file(normal_path)) {
      log_debug("Loading ", normal_path);
      _textures.emplace_back(gl.make_unique(normal_texture));
    }

    sprites.insert(
        s, std::unique_ptr<Sprite>(new Sprite{texture, normal_texture}));
  }

  // Tilesets are available as sprites as well.
  paths.clear();
  filesystem.list_pattern(paths, "/tiles/**.png");
  for (const std::string& s : paths) {
    std::string bare_path;
    filesystem.barename(bare_path, s);
    if (bare_path.substr(bare_path.length() - 7) == "_normal") {
      continue;
    }
    std::string data_path = bare_path + ".tile";
    std::string normal_path = bare_path + "_normal.png";

    log_debug("Loading ", bare_path);
    GlTexture2D texture = gl.make_texture(s);
    GlTexture2D normal_texture = filesystem.is_file(normal_path) ?
        gl.make_texture(normal_path) : _default_sprite->normal;

    _textures.emplace_back(gl.make_unique(texture));
    if (filesystem.is_file(normal_path)) {
      log_debug("Loading ", normal_path);
      _textures.emplace_back(gl.make_unique(normal_texture));
    }

    Tileset* tileset = new Tileset(Sprite{texture, normal_texture});
    if (filesystem.exists(data_path)) {
      log_debug("Loading ", data_path);
      tileset->load(filesystem, *this, data_path);
    }
    tilesets.insert(data_path, std::unique_ptr<Tileset>(tileset));
    sprites.insert(
        s, std::unique_ptr<Sprite>(new Sprite{texture, normal_texture}));
  }

  filesystem.read_file_with_includes(_default_script->contents,
                                     _default_script->path);
  // Scripts in the root directory are for inclusion only.
  paths.clear();
  filesystem.list_pattern(paths, "/scripts/*/**.lua");
  for (const std::string& s : paths) {
    log_debug("Loading ", s);
    LuaFile* lua_file = new LuaFile;
    lua_file->path = s;
    filesystem.read_file_with_includes(lua_file->contents, s);
    scripts.insert(s, std::unique_ptr<LuaFile>(lua_file));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/sounds/**.wav");
  for (const std::string& s : paths) {
    log_debug("Loading ", s);
    std::string data;
    filesystem.read_file(data, s);
    if (data.empty()) {
      continue;
    }

    sf::SoundBuffer* sound = new sf::SoundBuffer();
    if (!sound->loadFromMemory(data.data(), data.length())) {
      log_err("Couldn't load sound ", s);
      delete sound;
      continue;
    }
    sounds.insert(s, std::unique_ptr<sf::SoundBuffer>(sound));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/musics/**.ogg");
  for (const std::string& s : paths) {
    log_debug("Loading ", s);
    _music_data.emplace_back();
    filesystem.read_file(_music_data.back(), s);
    if (_music_data.back().empty()) {
      continue;
    }

    // TODO: should probably be buffered instead of loading from memory.
    // Filesystem needs a way to get a buffer rather than reading all at once.
    sf::Music* music = new sf::Music();
    if (!music->openFromMemory(_music_data.back().data(),
                               _music_data.back().length())) {
      log_err("Couldn't load music ", s);
      delete music;
      continue;
    }
    musics.insert(s, std::unique_ptr<sf::Music>(music));
  }

  // Maps depend on knowing the names of scripts, but grabbing data out of
  // scripts requires an actual map to be loaded for the GameStage, so this
  // needs to happen in-between.
  reload_cells_and_maps(filesystem);

  // Fill in the editor data of the scripts. Construct a fake GameStage so we
  // can access the functions.
  if (load_yedit_data) {
    RenderUtil fake_util(gl);
    GlUnique<GlFramebuffer> fake_framebuffer(
        gl.make_unique_framebuffer(y::ivec2{128, 128}, false, false));
    PhysicalFilesystem fake_filesystem("fake");
    GameStage fake_stage(*this, fake_filesystem, fake_util, *fake_framebuffer,
                         maps.get_names()[0], y::wvec2(), true);
    for (std::size_t i = 0; i < scripts.size(); ++i) {
      make_lua_file(scripts.get(i), fake_stage);
    }
  }
}

void Databank::reload_cells_and_maps(const Filesystem& filesystem)
{
  cells.clear();
  maps.clear();

  std::vector<std::string> paths;
  filesystem.list_pattern(paths, "/world/**.cell");
  for (const std::string& s : paths) {
    log_debug("Loading ", s);
    CellBlueprint* cell = new CellBlueprint();
    cell->load(filesystem, *this, s);
    cells.insert(s, std::unique_ptr<CellBlueprint>(cell));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/world/**.map");
  for (const std::string& s : paths) {
    log_debug("Loading ", s);
    CellMap* map = new CellMap();
    map->load(filesystem, *this, s);
    maps.insert(s, std::unique_ptr<CellMap>(map));
  }

  if (cells.get_names().empty() || maps.get_names().empty()) {
    make_default_map();
  }
}

void Databank::make_default_map()
{
  CellBlueprint* blueprint = new CellBlueprint();
  CellMap* map = new CellMap();
  map->set_coord(y::ivec2(), *blueprint);

  cells.insert(
      "/world/default.cell", std::unique_ptr<CellBlueprint>(blueprint));
  maps.insert("/world/default.map", std::unique_ptr<CellMap>(map));
}

void Databank::make_lua_file(LuaFile& file, GameStage& fake_stage)
{
  Script script(fake_stage, file.path, file.contents, y::wvec2(), y::wvec2());

  file.yedit_colour = y::fvec4{.5f, .5f, .5f, 1.f};
  if (script.has_function("yedit_colour")) {
    Script::lua_args out;
    script.call(out, "yedit_colour");
    if (out.size() >= 3) {
      file.yedit_colour[rr] = float(out[0].world);
      file.yedit_colour[gg] = float(out[1].world);
      file.yedit_colour[bb] = float(out[2].world);
    }
  }
}
