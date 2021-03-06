#include "modal.h"

#include "render/util.h"
#include "render/window.h"

#include <chrono>
#include <thread>
#include <SFML/Window.hpp>

UndoStack::UndoStack()
  : _save_offset(0)
  , _save_position_exists(true)
{
}

bool UndoStack::can_undo() const
{
  return !_undo_stack.empty();
}

bool UndoStack::can_redo() const
{
  return !_redo_stack.empty();
}

void UndoStack::new_action(std::unique_ptr<StackAction> action)
{
  StackAction* a = action.get();
  _undo_stack.emplace_back();
  _undo_stack.rbegin()->swap(action);

  if (a->is_noop()) {
    _undo_stack.pop_back();
    return;
  }
  a->redo();
  _redo_stack.clear();

  if (_save_offset >= 0) {
    ++_save_offset;
  }
  else {
    _save_position_exists = false;
  }
}

void UndoStack::undo()
{
  if (!can_undo()) {
    return;
  }

  (*_undo_stack.rbegin())->undo();
  _redo_stack.emplace_back();
  _undo_stack.rbegin()->swap(*_redo_stack.rbegin());
  _undo_stack.erase(_undo_stack.end() - 1);
  --_save_offset;
}

void UndoStack::redo()
{
  if (!can_redo()) {
    return;
  }

  (*_redo_stack.rbegin())->redo();
  _undo_stack.emplace_back();
  _redo_stack.rbegin()->swap(*_undo_stack.rbegin());
  _redo_stack.erase(_redo_stack.end() - 1);
  ++_save_offset;
}

void UndoStack::save_position()
{
  _save_offset = 0;
  _save_position_exists = true;
}

bool UndoStack::is_position_saved() const
{
  return _save_position_exists && !_save_offset;
}

Draggable::Draggable()
  : _drag(false)
{
}

Panel::Panel(const y::ivec2& origin, const y::ivec2& size, std::int32_t z_index)
  : _visible(true)
  , _origin(origin)
  , _size(size)
  , _z_index(z_index)
{
}

void Draggable::start_drag(const y::ivec2& start)
{
  _drag = true;
  _drag_start = start;
}

void Draggable::stop_drag()
{
  _drag = false;
}

bool Draggable::is_dragging() const
{
  return _drag;
}

const y::ivec2& Draggable::get_drag_start() const
{
  return _drag_start;
}

void Panel::set_origin(const y::ivec2& origin)
{
  _origin = origin;
}

void Panel::set_size(const y::ivec2& size)
{
  _size = size;
}

void Panel::set_visible(bool visible)
{
  _visible = visible;
}

const y::ivec2& Panel::get_origin() const
{
  return _origin;
}

const y::ivec2& Panel::get_size() const
{
  return _size;
}

bool Panel::is_visible() const
{
  return _visible;
}

bool Panel::order::operator()(Panel* a, Panel* b) const
{
  if (a->_z_index > b->_z_index) {
    return true;
  }
  if (a->_z_index < b->_z_index) {
    return false;
  }
  static std::less<Panel*> less;
  return less(b, a);
}

PanelUi::PanelUi(Modal& parent)
  : _parent(parent)
{
}

void PanelUi::add(Panel& panel)
{
  _panels.insert(&panel);
}

void PanelUi::remove(Panel& panel)
{
  _panels.erase(&panel);
  _mouse_over.erase(&panel);
}

bool PanelUi::event(const sf::Event& e)
{
  bool button =
      e.type == sf::Event::MouseButtonPressed ||
      e.type == sf::Event::MouseButtonReleased;
  bool wheel =
      e.type == sf::Event::MouseWheelMoved;
  bool enter =
      e.type == sf::Event::MouseEntered;
  bool leave =
      e.type == sf::Event::MouseLeft;
  bool move =
      e.type == sf::Event::MouseMoved || leave || enter;

  Panel* drag = get_drag_panel();

  // Just dispatch regular events.
  if (!wheel && !button && !move) {
    if (drag) {
      drag->event(e);
      return true;
    }
    for (Panel* panel : _panels) {
      if (panel->is_visible() && (panel->event(e) || panel->is_dragging())) {
        return true;
      }
    }
    return false;
  }

  if (leave) {
    if (!drag) {
      update_mouse_overs(false, {e.mouseMove.x, e.mouseMove.y});
    }
    return false;
  }

  // Mouse events need to be translated into panel coordinates.
  sf::Event f = e;
  if (enter) {
    f.type = sf::Event::MouseMoved;
  }

  auto& fx = wheel ? f.mouseWheel.x : button ? f.mouseButton.x : f.mouseMove.x;
  auto& fy = wheel ? f.mouseWheel.y : button ? f.mouseButton.y : f.mouseMove.y;
  y::ivec2 o{fx, fy};

  // Mouse drags take priority.
  if (drag) {
    fx = o[xx] - drag->get_origin()[xx];
    fy = o[yy] - drag->get_origin()[yy];
    drag->event(f);
    return true;
  }

  update_mouse_overs(true, o);

  for (Panel* panel : _panels) {
    fx = o[xx] - panel->get_origin()[xx];
    fy = o[yy] - panel->get_origin()[yy];
    if (y::in_region(y::ivec2{fx, fy}, y::ivec2{0, 0}, panel->get_size())) {
      if (panel->is_visible() && (panel->event(f) || panel->is_dragging())) {
        return !enter;
      }
    }
  }
  return false;
}

void PanelUi::update()
{
  // Mouse drags take priority.
  Panel* drag = get_drag_panel();
  if (drag) {
    drag->update();
    return;
  }
  for (Panel* panel : _panels) {
    panel->update();
    if (panel->is_dragging()) {
      return;
    }
  }
}

void PanelUi::draw(RenderUtil& util) const
{
  for (Panel* panel : _panels) {
    if (!panel->is_visible()) {
      continue;
    }
    // Transform rendering into panel coordinates.
    const y::ivec2& origin = panel->get_origin();
    const y::ivec2& size = panel->get_size();
    const y::fvec4& c = _mouse_over.find(panel) != _mouse_over.end() ?
        colour::outline : colour::dark_outline;

    util.iadd_translation(origin);
    panel->draw(util);
    util.irender_outline({-1, -1}, size + y::ivec2{2, 2}, c);
    util.iadd_translation(-origin);
  }
}

Panel* PanelUi::get_drag_panel() const
{
  for (Panel* panel : _panels) {
    if (panel->is_dragging()) {
      return panel;
    }
  }
  return nullptr;
}

void PanelUi::update_mouse_overs(bool in_window, const y::ivec2& v)
{
  bool old_any_over = !_mouse_over.empty();

  sf::Event e;
  bool first = true;
  for (Panel* panel : _panels) {
    bool old_over = _mouse_over.find(panel) != _mouse_over.end();
    bool new_over = in_window && first && panel->is_visible() &&
        y::in_region(v, panel->get_origin(), panel->get_size());

    y::ivec2 p = v - panel->get_origin();
    e.mouseMove.x = p[xx];
    e.mouseMove.y = p[yy];

    if (old_over && !new_over) {
      e.type = sf::Event::MouseLeft;
      panel->event(e);
      _mouse_over.erase(panel);
    }
    else if (new_over && !old_over) {
      e.type = sf::Event::MouseEntered;
      if (panel->event(e)) {
        first = false;
      }
      _mouse_over.insert(panel);
    }
  }

  bool new_any_over = !_mouse_over.empty();
  e.mouseMove.x = v[xx];
  e.mouseMove.y = v[yy];

  if (old_any_over && !new_any_over) {
    e.type = sf::Event::MouseEntered;
    _parent.event(e);
  }
  else if (new_any_over && !old_any_over) {
    e.type = sf::Event::MouseLeft;
    _parent.event(e);
  }
}

Modal::Modal()
  : _stack(nullptr)
  , _has_drawn_next(false)
  , _end(false)
  , _panel_ui(*this)
{
}

void Modal::push(std::unique_ptr<Modal> modal)
{
  _stack->push(std::move(modal));
}

bool Modal::has_next() const
{
  return _stack->has_next(*this);
}

void Modal::draw_next() const
{
  _has_drawn_next = true;
  _stack->draw_next(*this);
}

void Modal::end()
{
  _end = true;
}

bool Modal::is_ended() const
{
  return _end;
}

const UndoStack& Modal::get_undo_stack() const
{
  return _undo_stack;
}

UndoStack& Modal::get_undo_stack()
{
  return _undo_stack;
}

const PanelUi& Modal::get_panel_ui() const
{
  return _panel_ui;
}

PanelUi& Modal::get_panel_ui()
{
  return _panel_ui;
}

void Modal::set_stack(ModalStack& stack)
{
  _stack = &stack;
}

void Modal::clear_drawn_next() const
{
  _has_drawn_next = false;
}

bool Modal::has_drawn_next() const
{
  return _has_drawn_next;
}

void ModalStack::push(std::unique_ptr<Modal> modal)
{
  modal->set_stack(*this);
  _stack.emplace_back();
  _stack[_stack.size() - 1].swap(modal);
}

bool ModalStack::empty() const
{
  return _stack.empty();
}

void ModalStack::run(Window& window, RunTiming& run_timing)
{
  // TODO: mess with this some more? It's not very smart. It does seem to
  // correctly never drop below target FPS on a powerful enough GPU, but
  // has somewhat odd oscillating behaviour.
  typedef std::chrono::high_resolution_clock hrclock;

  hrclock clock;
  auto update_last(clock.now());
  auto draw_last(clock.now());
  auto accumulated_update_ticks(hrclock::duration::zero());
  auto accumulated_draw_ticks(hrclock::duration::zero());

  const std::size_t measurements = 64;
  std::list<std::size_t> update_measurements;
  std::list<std::size_t> draw_measurements;
  for (std::size_t i = 0; i < measurements; ++i) {
    update_measurements.emplace_back(0);
    draw_measurements.emplace_back(0);
  }

  while (!empty()) {
    auto ticks_per_update = std::chrono::duration_cast<hrclock::duration>(
        std::chrono::nanoseconds(
            std::size_t(1000000000.f /
                        run_timing.target_updates_per_second + .5f)));
    auto ticks_per_draw = std::chrono::duration_cast<hrclock::duration>(
        std::chrono::nanoseconds(
            std::size_t(1000000000.f /
                        run_timing.target_draws_per_second + .5f)));

    // Calculate the number of updates to do.
    std::size_t updates = 1;
    auto now(clock.now());
    if (run_timing.target_updates_per_second > 0.f) {
      accumulated_update_ticks += (now - update_last);

      updates = 0;
      while (accumulated_update_ticks >= ticks_per_update) {
        ++updates;
        accumulated_update_ticks -= ticks_per_update;
      }
      if (!updates) {
        run_timing.draws_this_cycle = 0;
        update_last = now;
        continue;
      }
    }
    update_last = now;
    run_timing.updates_this_cycle += updates;

    // Do the updates.
    for (std::size_t i = 0; i < updates; ++i) {
      auto time_start(clock.now());

      std::size_t top = _stack.size() - 1;
      sf::Event e;
      while (window.poll_event(e)) {
        if (e.type == sf::Event::Closed) {
          return;
        }
        event(e, top);
        if (clear_ended()) {
          break;
        }
      }
      update();
      if (clear_ended()) {
        break;
      }

      update_measurements.emplace_back(
          std::chrono::duration_cast<std::chrono::microseconds>(
              clock.now() - time_start).count());
      update_measurements.pop_front();
    }

    // Calculate whether to do a draw.
    now = clock.now();
    if (run_timing.target_draws_per_second > 0.f) {
      accumulated_draw_ticks += (now - draw_last);

      bool draw = false;
      while (accumulated_draw_ticks >= ticks_per_draw) {
        draw = true;
        accumulated_draw_ticks -= ticks_per_draw;
      }
      if (!draw) {
        draw_last = now;
        continue;
      }
    }
    draw_last = now;
    run_timing.draws_this_cycle += 1;

    // Do the draw.
    auto time_start(clock.now());
    draw(0);
    window.display();
    draw_measurements.emplace_back(
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock.now() - time_start).count());
    draw_measurements.pop_front();

    // Update the data.
    run_timing.updates_this_cycle = 0;
    run_timing.draws_this_cycle = 0;
    run_timing.us_per_update_inst = *update_measurements.rbegin();
    run_timing.us_per_draw_inst = *draw_measurements.rbegin();
    run_timing.us_per_update_avg = 0;
    run_timing.us_per_draw_avg = 0;
    for (std::size_t m : update_measurements) {
      run_timing.us_per_update_avg += m;
    }
    for (std::size_t m : draw_measurements) {
      run_timing.us_per_draw_avg += m;
    }
    run_timing.us_per_update_avg /= measurements;
    run_timing.us_per_draw_avg /= measurements;
    run_timing.us_per_frame_inst =
        run_timing.us_per_update_inst + run_timing.us_per_draw_inst;
    run_timing.us_per_frame_avg =
        run_timing.us_per_update_avg + run_timing.us_per_draw_avg;
  }
}

bool ModalStack::has_next(const Modal& modal) const
{
  for (std::size_t i = 0; i < _stack.size(); ++i) {
    if (_stack[i].get() == &modal) {
      return 1 + i < _stack.size();
    }
  }
  return false;
}

void ModalStack::draw_next(const Modal& modal) const
{
  for (std::size_t i = 0; i < _stack.size(); ++i) {
    if (_stack[i].get() == &modal && 1 + i < _stack.size()) {
      draw(1 + i);
    }
  }
}

void ModalStack::event(const sf::Event& e, std::size_t index)
{
  if (empty() || index >= _stack.size()) {
    return;
  }

  element& top = _stack[index];

  // Automatically handle undo/redo.
  if (e.type == sf::Event::KeyPressed &&
      e.key.code == sf::Keyboard::Z && e.key.control) {
    if (e.key.shift) {
      top->get_undo_stack().redo();
    }
    else {
      top->get_undo_stack().undo();
    }
    return;
  }

  // Hand off to dragging first, then UI, then non-dragging.
  if (top->is_dragging() || !top->get_panel_ui().event(e)) {
    top->event(e);
  }
}

void ModalStack::update()
{
  if (empty()) {
    return;
  }

  element& top = *(_stack.end() - 1);
  top->get_panel_ui().update();
  top->update();
}

void ModalStack::draw(std::size_t index) const
{
  for (std::size_t i = index; i < _stack.size(); ++i) {
    const auto& modal = _stack[i];
    modal->clear_drawn_next();
    modal->draw();
    if (modal->has_drawn_next()) {
      break;
    }
  }
}

bool ModalStack::clear_ended()
{
  for (std::size_t i = 0; i < _stack.size(); ++i) {
    if (!_stack[i]->is_ended()) {
      continue;
    }
    _stack.erase(_stack.begin() + i);
    --i;
  }
  return empty();
}
