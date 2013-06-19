#include "map_editor.h"

#include "databank.h"
#include "gl_util.h"
#include "window.h"

#include <SFML/Window.hpp>

MapEditor::MapEditor(Databank& bank, RenderUtil& util, CellMap& map)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _yedit_scene_program(util.get_gl().make_unique_program({
        "/shaders/yedit_scene.v.glsl",
        "/shaders/yedit_scene.f.glsl"}))
  , _camera_drag(false)
  , _light_drag(false)
  , _zoom(2)
  , _hover{-1, -1}
  , _light_direction{1.f, 1.f}
  , _brush_panel(bank, _tile_brush)
  , _tile_panel(bank, _tile_brush)
  , _script_panel(bank)
  , _layer_panel(_layer_status)
  , _minimap_panel(map, _camera, _zoom)
{
  get_panel_ui().add(_brush_panel);
  get_panel_ui().add(_tile_panel);
  get_panel_ui().add(_script_panel);
  get_panel_ui().add(_layer_panel);
  get_panel_ui().add(_minimap_panel);
}

void MapEditor::event(const sf::Event& e)
{
  // Update hover position.
  if (e.type == sf::Event::MouseMoved) {
    _hover = y::ivec2(y::fvec2(float(e.mouseMove.x), float(e.mouseMove.y)) /
                      Zoom::array[_zoom]);
  }
  if (e.type == sf::Event::MouseLeft) {
    _hover = {-1, -1};
  }
  if (e.type == sf::Event::MouseWheelMoved) {
    _layer_panel.event(e);
  }

  bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
  bool control = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

  // Start camera drag.
  if (e.type == sf::Event::MouseButtonPressed && !is_dragging() &&
      (shift || e.mouseButton.button == sf::Mouse::Middle)) {
    start_drag(_hover);
    _camera_drag = e.mouseButton.button != sf::Mouse::Right;
    _light_drag = !_camera_drag;
  }
  else if (e.type == sf::Event::MouseButtonPressed && !is_dragging() &&
           (e.mouseButton.button == sf::Mouse::Left ||
            e.mouseButton.button == sf::Mouse::Right)) {
    // Start tile draw or pick.
    if (!is_script_layer()) {
      start_drag(camera_to_world(_hover).euclidean_div(Tileset::tile_size));
      if (e.mouseButton.button == sf::Mouse::Left) {
        _tile_edit_style = control;
        _tile_edit_action = y::move_unique(
            new TileEditAction(_map, _layer_panel.get_layer()));
      }
    }
    // Start script region create or move.
    else if (e.mouseButton.button == sf::Mouse::Left) {
      start_drag(get_hover_world());
      if (control || !_map.has_script_at(get_hover_world())) {
        _script_add_action = y::move_unique(
            new ScriptAddAction(_map, get_hover_world(), get_hover_world(),
                _script_panel.get_script()));
      }
    }
    // Pick script.
    else if (e.mouseButton.button == sf::Mouse::Right &&
             _map.has_script_at(get_hover_world())) {
      _script_panel.set_script(_map.get_script_at(get_hover_world()).path);
    }
  }

  if (e.type == sf::Event::MouseButtonReleased && is_dragging()) {
    // End draw and commit tile edit.
    if (_tile_edit_action) {
      get_undo_stack().new_action(y::move_unique(_tile_edit_action));
    }
    // End drag and add script.
    else if (_script_add_action) {
      get_undo_stack().new_action(y::move_unique(_script_add_action));
    }
    else if (is_script_layer()) {
      get_undo_stack().new_action(y::move_unique(
          new ScriptMoveAction(_map, get_drag_start(), get_hover_world())));
    }
    // End pick and copy tiles.
    else if (!_camera_drag && !_light_drag) {
      copy_drag_to_brush();
    }
    stop_drag();
    _camera_drag = false;
    _light_drag = false;
  }

  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  CellBlueprint* cell = _map.get_coord(get_hover_cell());
  y::int32 old_zoom = _zoom;
  switch (e.key.code) {
    // Quit!
    case sf::Keyboard::Escape:
      end();
      break;

    // Kick off game.
    case sf::Keyboard::Return:
      if (shift) {
        y::string call = "bin/yugen " + _bank.maps.get_name(_map) +
                         " " + y::to_string(get_hover_world()[xx]) +
                         " " + y::to_string(get_hover_world()[yy]);
        std::cout << call << " exited with code " << system(call.c_str()) <<
            std::endl;
      }
      break;

    // Rename cell.
    case sf::Keyboard::R:
      if (is_mouse_on_screen() && cell) {
        const y::string& name =
            _bank.cells.get_name(*cell);
        push(y::move_unique(new TextInputModal(
            _util, name, _input_result, "Rename cell " + name + " to:")));
      }
      break;

    // New cell.
    case sf::Keyboard::N:
      if (is_mouse_on_screen() && !cell) {
        push(y::move_unique(new TextInputModal(
            _util, "/world/new.cell", _input_result, "Add cell using name:")));
      }
      break;

    // Remove cell or script.
    case sf::Keyboard::Delete:
      if (!is_script_layer() && _map.is_coord_used(get_hover_cell())) {
        get_undo_stack().new_action(y::move_unique(
            new CellRemoveAction(_bank, _map, get_hover_cell())));
      }
      if (is_script_layer() && _map.has_script_at(get_hover_world())) {
        get_undo_stack().new_action(y::move_unique(
            new ScriptRemoveAction(_map, get_hover_world())));
      }
      break;

    // Zoom in.
    case sf::Keyboard::Z:
      ++_zoom;
      y::clamp(_zoom, 0, y::int32(Zoom::array.size()));
      _hover = y::ivec2(y::fvec2(_hover) *
                        Zoom::array[old_zoom] / Zoom::array[_zoom]);
      break;

    // Zoom out.
    case sf::Keyboard::X:
      --_zoom;
      y::clamp(_zoom, 0, y::int32(Zoom::array.size()));
      _hover = y::ivec2(y::fvec2(_hover) *
                        Zoom::array[old_zoom] / Zoom::array[_zoom]);
      break;

    // Unused key.
    default: {}
  }
}

void MapEditor::update()
{
  // Layout UI.
  const Resolution& r = _util.get_window().get_mode();
  const y::ivec2 spacing = RenderUtil::from_grid({0, 1});

  bool script_layer = _layer_panel.get_layer() > Cell::foreground_layers;
  _tile_panel.set_visible(!script_layer);
  _brush_panel.set_visible(!script_layer);
  _script_panel.set_visible(script_layer);
  Panel& top_panel = script_layer ?
      (Panel&)_script_panel : (Panel&)_tile_panel;

  top_panel.set_origin(RenderUtil::from_grid());
  _layer_panel.set_origin(top_panel.get_origin() + spacing +
                          y::ivec2{0, top_panel.get_size()[yy]});
  _brush_panel.set_origin(_layer_panel.get_origin() + spacing +
                          y::ivec2{0, _layer_panel.get_size()[yy]});
  _minimap_panel.set_origin(r.size - _minimap_panel.get_size() -
                            RenderUtil::from_grid());

  // Move camera with arrow keys.
  y::int32 speed = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) ? 32 : 8;
  y::ivec2 d;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    --d[yy];
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    ++d[yy];
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    --d[xx];
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    ++d[xx];
  }
  _camera += speed * d;

  // Update hover and status text.
  if (!is_mouse_on_screen()) {
    return;
  }

  _layer_status.clear();
  y::sstream ss;
  ss << get_hover_cell() << " : " << get_hover_tile() <<
      " [" << Zoom::array[_zoom] << "X]";
  _layer_status.emplace_back(ss.str());
  if (_map.is_coord_used(get_hover_cell())) {
    _layer_status.emplace_back(
        _bank.cells.get_name(*_map.get_coord(get_hover_cell())));
  }

  // Drag camera.
  if (_camera_drag) {
    _camera += get_drag_start() - _hover;
    start_drag(_hover);
    return;
  }

  // Drag light.
  if (_light_drag) {
    y::fvec2 d = y::fvec2(get_drag_start() - _hover);
    if (d != y::fvec2()) {
      _light_direction = d / y::max(y::abs(d[xx]), y::abs(d[yy]));
    }
  }

  // Rename or add cell.
  if (_input_result.success) {
    const y::string& result = _input_result.result;
    if (result.substr(0, 7) == "/world/" &&
        result.substr(result.length() - 5) == ".cell") {
      if (_map.is_coord_used(get_hover_cell())) {
        if (!_bank.cells.is_name_used(result)) {
          get_undo_stack().new_action(y::move_unique(
              new CellRenameAction(_bank, _map, get_hover_cell(), result)));
        }
      }
      else {
        get_undo_stack().new_action(y::move_unique(
            new CellAddAction(_bank, _map, get_hover_cell(), result)));
      }
    }
    _input_result.success = false;
  }

  if (!is_dragging()) {
    return;
  }
  // Continuous tile drawing.
  if (_tile_edit_action) {
    copy_brush_to_action();
  }
  // Script sizing.
  if (_script_add_action) {
    _script_add_action->min = y::min(get_drag_start(), get_hover_world());
    _script_add_action->max = y::max(get_drag_start(), get_hover_world());
  }
}

void MapEditor::draw() const
{
  _util.set_scale(Zoom::array[_zoom]);

  // Set up buffers the same size as the area we're to draw.
  const y::ivec2& screen_size = _util.get_window().get_mode().size;
  GlUnique<GlFramebuffer> colourbuffer(
      _util.get_gl().make_unique_framebuffer(screen_size, true));
  GlUnique<GlFramebuffer> normalbuffer(
      _util.get_gl().make_unique_framebuffer(screen_size, false));

  // Draw cell contents.
  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    // Draw layer contents to colour and normal buffers.
    colourbuffer->bind(true, true);
    RenderBatch batch;
    for (auto it = _map.get_cartesian(); it; ++it) {
      draw_cell_layer(batch, *it, layer, false);
    }
    _util.render_batch(batch);

    normalbuffer->bind(true, true);
    batch.clear();
    for (auto it = _map.get_cartesian(); it; ++it) {
      draw_cell_layer(batch, *it, layer, true);
    }
    _util.render_batch(batch);

    // Render scene.
    _util.get_gl().bind_window(false, false);
    _yedit_scene_program->bind();
    _yedit_scene_program->bind_attribute("position", _util.quad_vertex());
    _yedit_scene_program->bind_uniform("colourbuffer", *colourbuffer);
    _yedit_scene_program->bind_uniform("normalbuffer", *normalbuffer);
    _yedit_scene_program->bind_uniform("light_direction", _light_direction);
    _yedit_scene_program->bind_uniform("layer_colour", colour_for_layer(layer));
    _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
  }

  _util.get_gl().bind_window(false, true);

  // Draw cell-grid.
  y::ivec2 min = y::min(_map.get_boundary_min(), get_hover_cell());
  y::ivec2 max = y::max(
      _map.get_boundary_max(), y::ivec2{1, 1} + get_hover_cell());
  for (auto it = y::cartesian(min, max); it; ++it) {
    if (_map.is_coord_used(*it) ||
        (is_mouse_on_screen() && *it == get_hover_cell())) {
      _util.irender_outline(
          world_to_camera(*it * Tileset::tile_size * Cell::cell_size),
          Tileset::tile_size * Cell::cell_size, colour::panel);
    }
  }

  // Draw script regions.
  if (is_script_layer()) {
    draw_scripts();
  }

  // Draw native resolution indicator.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    const Resolution& r = _util.get_window().get_mode();
    _util.irender_outline(r.size / 2 - RenderUtil::native_size / 2,
                          RenderUtil::native_size, colour::outline);
  }

  // Draw cursor.
  draw_cursor();

  // Draw UI.
  _util.set_scale(1.f);
  get_panel_ui().draw(_util);
}

y::ivec2 MapEditor::world_to_camera(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v - _camera + y::ivec2(y::fvec2(r.size / 2) / Zoom::array[_zoom]);
}

y::ivec2 MapEditor::camera_to_world(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v - y::ivec2(y::fvec2(r.size / 2) / Zoom::array[_zoom]) + _camera;
}

void MapEditor::copy_drag_to_brush()
{
  y::ivec2 min = y::min(get_drag_start(), get_hover_world());
  y::ivec2 max = y::max(get_drag_start(), get_hover_world());
  for (auto it = y::cartesian(min, y::ivec2{1, 1} + max); it; ++it) {
    y::ivec2 u = *it - min;
    if (!(u < TileBrush::max_size)) {
      continue;
    }

    y::ivec2 c = it->euclidean_div(Cell::cell_size);
    y::ivec2 t = it->euclidean_mod(Cell::cell_size);
    Tile& e = _tile_brush.get(u);

    if (!_map.is_coord_used(c)) {
      e.tileset = y::null;
      e.index = 0;
    }
    else {
      CellBlueprint* cell = _map.get_coord(c);
      const Tile& tile = cell->get_tile(_layer_panel.get_layer(), t);
      e.tileset = tile.tileset;
      e.index = tile.index;
    }
  }
  _tile_brush.size = y::min(y::ivec2{1, 1} + max - min, TileBrush::max_size);
}

void MapEditor::copy_brush_to_action() const
{
  y::ivec2 min;
  y::ivec2 max = _tile_brush.size;
  y::ivec2 flip{0, 0};
  if (_tile_edit_style) {
    _tile_edit_action->edits.clear();
    min = y::min(get_drag_start(), get_hover_world());
    max = y::ivec2{1, 1} + y::max(get_drag_start(), get_hover_world());
    flip = {get_drag_start()[xx] > get_hover_world()[xx],
            get_drag_start()[yy] > get_hover_world()[yy]};
  }

  for (auto it = y::cartesian(min, max); it; ++it) {
    y::ivec2 v = _tile_edit_style ? *it : *it + get_hover_world();
    y::ivec2 c = v.euclidean_div(Cell::cell_size);
    y::ivec2 t = v.euclidean_mod(Cell::cell_size);
    if (!_map.is_coord_used(c)) {
      continue;
    }

    y::ivec2 u = _tile_edit_style ?
        (*it - min) * (y::ivec2{1, 1} - flip) + (*it - max) * flip :
        *it + get_hover_world() - get_drag_start();
    _tile_edit_action->set_tile(
        c, t, _tile_brush.get(u.euclidean_mod(_tile_brush.size)));
  }
}

const y::fvec4& MapEditor::colour_for_layer(y::int32 layer) const
{
  y::int32 active_layer = _layer_panel.get_layer();

  return active_layer > Cell::foreground_layers ? colour::white :
      layer < active_layer ? colour::dark :
      layer == active_layer ? colour::white :
      colour::transparent;
}

void MapEditor::draw_cell_layer(
    RenderBatch& batch,
    const y::ivec2& coord, y::int32 layer, bool normal) const
{
  if (!_map.is_coord_used(coord)) {
    return;
  }
  const CellBlueprint* cell = _map.get_coord(coord);

  bool edit = is_dragging() &&
      _tile_edit_action && _tile_edit_action->layer == layer;

  for (auto it = y::cartesian(Cell::cell_size); it; ++it) {
    // In the middle of a TileEditAction we need to render the
    // uncommited drawing.
    y::map<TileEditAction::key, TileEditAction::entry>::const_iterator jt;
    if (edit) {
      jt = _tile_edit_action->edits.find(y::make_pair(coord, *it));
    }
    const Tile& t = edit && jt != _tile_edit_action->edits.end() ?
        jt->second.new_tile : cell->get_tile(layer, *it);

    // Draw the tile.
    if (!t.tileset) {
      continue;
    }

    y::ivec2 camera = world_to_camera(
        Tileset::tile_size * (*it + coord * Cell::cell_size));

    const auto& texture = normal ?
        t.tileset->get_texture().normal : t.tileset->get_texture().texture;
    batch.iadd_sprite(texture, Tileset::tile_size,
                      camera, t.tileset->from_index(t.index), 0.f,
                      colour::white);
  }
}

void MapEditor::draw_scripts() const
{
  // Script region the mouse is hovering over.
  const ScriptBlueprint* hover_script =
      !is_dragging() && _map.has_script_at(get_hover_world()) ?
          &_map.get_script_at(get_hover_world()) : y::null;

  // Script being dragged.
  const ScriptBlueprint* drag_script = is_dragging() && !_script_add_action ?
      &_map.get_script_at(get_drag_start()) : y::null;

  for (const ScriptBlueprint& s : _map.get_scripts()) {
    // Need to show an uncommitted script drag.
    y::ivec2 off = &s == drag_script ?
        get_hover_world() - get_drag_start() : y::ivec2();
    y::ivec2 min = world_to_camera((s.min + off) * Tileset::tile_size);
    y::ivec2 max = world_to_camera((s.max + off) * Tileset::tile_size);
    _util.irender_fill(min, Tileset::tile_size + max - min,
                       colour::highlight);
    _util.irender_outline(min, Tileset::tile_size + max - min,
                          colour::outline);
    _util.irender_text(
        s.path, min, &s == hover_script || &s == drag_script ?
            colour::select : colour::item);
  }
}

void MapEditor::draw_cursor() const
{
  if (is_dragging() && !_script_add_action && is_script_layer()) {
    return;
  }

  y::ivec2 cursor_end = get_hover_world() + _tile_brush.size - y::ivec2{1, 1};
  // Draw a rectangle-dragging-out cursor.
  if (is_dragging() && !_camera_drag && !_light_drag &&
      (!_tile_edit_action || _tile_edit_style)) {
    y::ivec2 min = y::min(get_drag_start(), get_hover_world());
    y::ivec2 max = y::max(get_drag_start(), get_hover_world());
    _util.irender_outline(
        world_to_camera(min * Tileset::tile_size),
        Tileset::tile_size * (y::ivec2{1, 1} + max - min), colour::hover);
  }
  // Draw a fixed-size cursor from the top-left.
  else if (is_mouse_on_screen() &&
           (_map.is_coord_used(get_hover_cell()) ||
            _map.is_coord_used(cursor_end.euclidean_div(Cell::cell_size)))) {
    _util.irender_outline(
        world_to_camera(get_hover_world() * Tileset::tile_size),
        Tileset::tile_size * (is_script_layer() ?
                              y::ivec2{1, 1} : _tile_brush.size),
        colour::hover);
  }
}

bool MapEditor::is_script_layer() const
{
  return _layer_panel.get_layer() > Cell::foreground_layers;
}

bool MapEditor::is_mouse_on_screen() const
{
  return _hover >= y::ivec2();
}

y::ivec2 MapEditor::get_hover_world() const
{
  return camera_to_world(_hover).euclidean_div(Tileset::tile_size);
}

y::ivec2 MapEditor::get_hover_tile() const
{
  return get_hover_world().euclidean_mod(Cell::cell_size);
}

y::ivec2 MapEditor::get_hover_cell() const
{
  return get_hover_world().euclidean_div(Cell::cell_size);
}
