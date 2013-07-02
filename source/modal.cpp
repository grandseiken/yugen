#include "modal.h"
#include "render_util.h"
#include "window.h"

#include <chrono>
#include <thread>
#include <SFML/Window.hpp>

bool UndoStack::can_undo() const
{
  return !_undo_stack.empty();
}

bool UndoStack::can_redo() const
{
  return !_redo_stack.empty();
}

void UndoStack::new_action(y::unique<StackAction> action)
{
  action->redo();
  _undo_stack.emplace_back();
  (_undo_stack.end() - 1)->swap(action);
  _redo_stack.clear();
}

void UndoStack::undo()
{
  if (!can_undo()) {
    return;
  }

  (*(_undo_stack.end() - 1))->undo();
  _redo_stack.emplace_back();
  (_undo_stack.end() - 1)->swap(*(_redo_stack.end() - 1));
  _undo_stack.erase(_undo_stack.end() - 1);
}

void UndoStack::redo()
{
  if (!can_redo()) {
    return;
  }

  (*(_redo_stack.end() - 1))->redo();
  _undo_stack.emplace_back();
  (_redo_stack.end() - 1)->swap(*(_undo_stack.end() - 1));
  _redo_stack.erase(_redo_stack.end() - 1);
}

Draggable::Draggable()
  : _drag(false)
{
}

Panel::Panel(const y::ivec2& origin, const y::ivec2& size, y::int32 z_index)
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
    if (y::ivec2{fx, fy}.in_region({0, 0}, panel->get_size())) {
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
  return y::null;
}

void PanelUi::update_mouse_overs(bool in_window, const y::ivec2& v)
{
  bool old_any_over = !_mouse_over.empty();

  sf::Event e;
  bool first = true;
  for (Panel* panel : _panels) {
    bool old_over = _mouse_over.find(panel) != _mouse_over.end();
    bool new_over = in_window && first && panel->is_visible() &&
        v.in_region(panel->get_origin(), panel->get_size());

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
  : _stack(y::null)
  , _has_drawn_next(false)
  , _end(false)
  , _panel_ui(*this)
{
}

void Modal::push(y::unique<Modal> modal)
{
  _stack->push(y::move_unique(modal));
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

void ModalStack::push(y::unique<Modal> modal)
{
  modal->set_stack(*this);
  _stack.emplace_back();
  _stack[_stack.size() - 1].swap(modal);
}

bool ModalStack::empty() const
{
  return _stack.empty();
}

void ModalStack::run(
    Window& window, float updates_per_second, float draws_per_second)
{
  // TODO: mess with this some more. It's not very smart. Parallelise?
  typedef std::chrono::high_resolution_clock hrclock;
  auto ticks_per_update = std::chrono::duration_cast<hrclock::duration>(
      std::chrono::nanoseconds(
          y::size(1000000000.f / updates_per_second + .5f)));
  auto ticks_per_draw = std::chrono::duration_cast<hrclock::duration>(
      std::chrono::nanoseconds(
          y::size(1000000000.f / draws_per_second + .5f)));

  hrclock clock;
  auto update_last(clock.now());
  auto draw_last(clock.now());
  hrclock::duration accumulated_update_ticks(hrclock::duration::zero());
  hrclock::duration accumulated_draw_ticks(hrclock::duration::zero());

  while (!empty()) {
    y::size updates = 1;
    if (updates_per_second > 0.f) {
      auto now(clock.now());
      accumulated_update_ticks += (now - update_last);
      update_last = now;

      updates = 0;
      while (accumulated_update_ticks >= ticks_per_update) {
        ++updates;
        accumulated_update_ticks -= ticks_per_update;
      }
      if (!updates) {
        continue;
      }
    }

    for (y::size i = 0; i < updates; ++i) {
      y::size top = _stack.size() - 1;
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
    }

    if (draws_per_second > 0.f) {
      auto now(clock.now());
      accumulated_draw_ticks += (now - draw_last);
      draw_last = now;

      bool draw = false;
      while (accumulated_draw_ticks >= ticks_per_draw) {
        draw = true;
        accumulated_draw_ticks -= ticks_per_draw;
      }
      if (!draw) {
        continue;
      }
    }

    draw(0);
    window.display();
  }
}

bool ModalStack::has_next(const Modal& modal) const
{
  for (y::size i = 0; i < _stack.size(); ++i) {
    if (_stack[i].get() == &modal) {
      return 1 + i < _stack.size();
    }
  }
  return false;
}

void ModalStack::draw_next(const Modal& modal) const
{
  for (y::size i = 0; i < _stack.size(); ++i) {
    if (_stack[i].get() == &modal && 1 + i < _stack.size()) {
      draw(1 + i);
    }
  }
}

void ModalStack::event(const sf::Event& e, y::size index)
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

void ModalStack::draw(y::size index) const
{
  for (y::size i = index; i < _stack.size(); ++i) {
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
  for (y::size i = 0; i < _stack.size(); ++i) {
    if (!_stack[i]->is_ended()) {
      continue;
    }
    _stack.erase(_stack.begin() + i);
    --i;
  }
  return empty();
}
