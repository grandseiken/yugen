#include "modal.h"
#include "render_util.h"
#include "window.h"

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
  _undo_stack.push_back(Element());
  (_undo_stack.end() - 1)->swap(action);
  _redo_stack.clear();
}

void UndoStack::undo()
{
  if (!can_undo()) {
    return;
  }

  (*(_undo_stack.end() - 1))->undo();
  _redo_stack.push_back(Element());
  (_undo_stack.end() - 1)->swap(*(_redo_stack.end() - 1));
  _undo_stack.erase(_undo_stack.end() - 1);
}

void UndoStack::redo()
{
  if (!can_undo()) {
    return;
  }

  (*(_redo_stack.end() - 1))->redo();
  _undo_stack.push_back(Element());
  (_redo_stack.end() - 1)->swap(*(_undo_stack.end() - 1));
  _redo_stack.erase(_redo_stack.end() - 1);
}

Panel::Panel(const y::ivec2& origin, const y::ivec2& size, y::int32 z_index)
  : _origin(origin)
  , _size(size)
  , _z_index(z_index)
  , _drag(false)
{
}

void Panel::set_origin(const y::ivec2& origin)
{
  _origin = origin;
}

void Panel::set_size(const y::ivec2& size)
{
  _size = size;
}

const y::ivec2& Panel::get_origin() const
{
  return _origin;
}

const y::ivec2& Panel::get_size() const
{
  return _size;
}

void Panel::start_drag(const y::ivec2& start)
{
  _drag = true;
  _drag_start = start;
}

void Panel::stop_drag()
{
  _drag = false;
}

bool Panel::is_dragging() const
{
  return _drag;
}

const y::ivec2& Panel::get_drag_start() const
{
  return _drag_start;
}

bool Panel::Order::operator()(Panel* a, Panel* b) const
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
      if (panel->event(e) || panel->is_dragging()) {
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

  int& fx = wheel ? f.mouseWheel.x : button ? f.mouseButton.x : f.mouseMove.x;
  int& fy = wheel ? f.mouseWheel.y : button ? f.mouseButton.y : f.mouseMove.y;
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
      if (panel->event(f) || panel->is_dragging()) {
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
    // Transform rendering into panel coordinates.
    const y::ivec2& origin = panel->get_origin();
    const y::ivec2& size = panel->get_size();
    Colour c = _mouse_over.find(panel) != _mouse_over.end() ?
      Colour(.7f, .7f, .7f, 1.f) : Colour(.3f, .3f, .3f, 1.f);

    util.add_translation(origin);
    panel->draw(util);
    util.render_outline({-1, -1}, size + y::ivec2{2, 2}, c);
    util.add_translation(-origin);
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
  bool first = true;
  for (Panel* panel : _panels) {
    bool old_over = _mouse_over.find(panel) != _mouse_over.end();
    bool new_over = in_window && first &&
        v.in_region(panel->get_origin(), panel->get_size());

    y::ivec2 p = v - panel->get_origin();
    if (old_over && !new_over) {
      sf::Event e;
      e.type = sf::Event::MouseLeft;
      e.mouseMove.x = p[xx];
      e.mouseMove.y = p[yy];
      panel->event(e);
      _mouse_over.erase(panel);
    }
    else if (new_over && !old_over) {
      sf::Event e;
      e.type = sf::Event::MouseEntered;
      e.mouseMove.x = p[xx];
      e.mouseMove.y = p[yy];
      if (panel->event(e)) {
        first = false;
      }
      _mouse_over.insert(panel);
    }
  }
}

Modal::Modal()
  : _end(false)
{
}

void Modal::push(y::unique<Modal> modal)
{
  _stack->push(y::move_unique(modal));
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

void ModalStack::push(y::unique<Modal> modal)
{
  modal->set_stack(*this);
  _stack.push_back(Element());
  _stack[_stack.size() - 1].swap(modal);
}

bool ModalStack::empty() const
{
  return _stack.empty();
}

void ModalStack::run(Window& window)
{
  while (!empty()) {
    sf::Event e;
    while (window.poll_event(e)) {
      if (e.type == sf::Event::Closed) {
        return;
      }
      event(e);
      if (clear_ended()) {
        break;
      }
    }
    update();
    if (clear_ended()) {
      break;
    }
    draw();
    window.display();
  }
}

void ModalStack::event(const sf::Event& e)
{
  if (empty()) {
    return;
  }

  // Automatically handle undo/redo.
  if (e.type == sf::Event::KeyPressed &&
      e.key.code == sf::Keyboard::Z && e.key.control) {
    if (e.key.shift) {
      _stack[_stack.size() - 1]->get_undo_stack().redo();
    }
    else {
      _stack[_stack.size() - 1]->get_undo_stack().undo();
    }
    return;
  }

  // Hand off to UI first.
  if (!_stack[_stack.size() - 1]->get_panel_ui().event(e)) {
    _stack[_stack.size() - 1]->event(e);
  }
}

void ModalStack::update()
{
  if (empty()) {
    return;
  }
  _stack[_stack.size() - 1]->get_panel_ui().update();
  _stack[_stack.size() - 1]->update();
}

void ModalStack::draw() const
{
  for (const auto& modal : _stack) {
    modal->draw();
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
