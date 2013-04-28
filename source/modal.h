#ifndef MODAL_H
#define MODAL_H

#include "common.h"
#include "vector.h"

namespace sf {
  class Event;
}
class Modal;
class ModalStack;
class RenderUtil;
class Window;

// A generic action that can be done or undone.
class StackAction : public y::no_copy {
public:

  virtual ~StackAction() {}

  virtual void undo() const = 0;
  virtual void redo() const = 0;

};

// Class for storing and using StackAction objects.
class UndoStack : public y::no_copy {
public:

  bool can_undo() const;
  bool can_redo() const;

  // Called by the Modal to execute and store a new StackAction.
  void new_action(y::unique<StackAction> action);

  // Undo and redo are handled automatically by the ModalStack.
  void undo();
  void redo();

private:

  typedef y::unique<StackAction> Element;
  typedef y::vector<Element> Stack;
  Stack _undo_stack;
  Stack _redo_stack;

};

// UI element that can handle a mouse drag.
class Draggable {
public:

  Draggable();
  virtual ~Draggable() {}

  void start_drag(const y::ivec2& start);
  void stop_drag();
  bool is_dragging() const;
  const y::ivec2& get_drag_start() const;

private:

  bool _drag;
  y::ivec2 _drag_start;

};

// A panel in the UI.
class Panel : public y::no_copy, public Draggable {
public:

  Panel(const y::ivec2& origin, const y::ivec2& size, y::int32 z_index = 0);
  virtual ~Panel() {}

  void set_origin(const y::ivec2& origin);
  void set_size(const y::ivec2& size);

  const y::ivec2& get_origin() const;
  const y::ivec2& get_size() const;

  virtual bool event(const sf::Event& e) = 0;
  virtual void update() = 0;
  virtual void draw(RenderUtil& util) const = 0;

  struct Order {
    bool operator()(Panel* a, Panel* b) const;
  };

private:

  y::ivec2 _origin;
  y::ivec2 _size;
  y::int32 _z_index;

};

// A set of panels making up a complete UI layer.
class PanelUi : public y::no_copy {
public:

  PanelUi(Modal& parent);

  // Add and remove panels. The caller of these functions must ensure that the
  // panel is not destroyed before it is removed.
  void add(Panel& panel);
  void remove(Panel& panel);

  // Returns true if the event is swallowed. Called automatically by ModalStack.
  bool event(const sf::Event& e);

  // Called automatically by ModalStack.
  void update();

  // Not called automatically by the ModalStack. It must be called explicitly by
  // the Modal when the UI should be drawn.
  void draw(RenderUtil& util) const;

private:

  // Find a Panel which is in the middle of a drag (or null).
  Panel* get_drag_panel() const;

  // Set mouse target, and potentially fire off MouseEntered/MouseLeft events.
  void update_mouse_overs(bool in_window, const y::ivec2& v);

  Modal& _parent;

  typedef y::ordered_set<Panel*, Panel::Order> PanelSet;
  PanelSet _panels;
  PanelSet _mouse_over;

};

// Interface to an entry in the ModalStack.
class Modal : public y::no_copy, public Draggable {
public:

  Modal();
  virtual ~Modal() {}

  // Virtual interface.
  virtual void event(const sf::Event& e) = 0;
  virtual void update() = 0;
  virtual void draw() const = 0;

  // Push a new mode onto the top of the enclosing stack.
  void push(y::unique<Modal> modal);

  // Signal that this mode should exit. Control returns to the mode behind.
  void end();
  bool is_ended() const;

  // Get helpers.
  const UndoStack& get_undo_stack() const;
  /***/ UndoStack& get_undo_stack();
  const PanelUi& get_panel_ui() const;
  /***/ PanelUi& get_panel_ui();

private:

  friend class ModalStack;
  void set_stack(ModalStack& stack);

  ModalStack* _stack;
  bool _end;

  UndoStack _undo_stack;
  PanelUi _panel_ui;

};

// A stack of modes. The topmost mode receives all events and controls the
// program flow. Modes are rendered from back to front.
class ModalStack {
public:

  void push(y::unique<Modal> modal);
  bool empty() const;

  // Run until the stack is empty. Push a mode onto the stack first.
  void run(Window& window);

private:

  void event(const sf::Event& e, y::size index);
  void update();
  void draw() const;
  bool clear_ended();

  typedef y::unique<Modal> Element;
  typedef y::vector<Element> Stack;
  Stack _stack;

};

#endif
