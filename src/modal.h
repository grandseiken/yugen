#ifndef MODAL_H
#define MODAL_H

#include "vec.h"
#include <memory>
#include <set>
#include <vector>

namespace sf {
  class Event;
}
class Modal;
class ModalStack;
class RenderUtil;
class Window;

// A generic action that can be done or undone.
class StackAction {
public:

  virtual ~StackAction() {}

  virtual void undo() const = 0;
  virtual void redo() const = 0;
  // Override to return true if the action is trivial and should be discarded.
  virtual bool is_noop() const = 0;

};

// Class for storing and using StackAction objects.
class UndoStack {
public:

  UndoStack();

  bool can_undo() const;
  bool can_redo() const;

  // Called by the Modal to execute and store a new StackAction.
  void new_action(std::unique_ptr<StackAction> action);

  // Undo and redo are handled automatically by the ModalStack.
  void undo();
  void redo();

  // Saves the current position in the stack, so we know if anything has changed
  // since the last save.
  void save_position();
  bool is_position_saved() const;

private:

  typedef std::unique_ptr<StackAction> element;
  typedef std::vector<element> stack;
  stack _undo_stack;
  stack _redo_stack;

  std::int32_t _save_offset;
  bool _save_position_exists;

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
class Panel : public Draggable {
public:

  Panel(const y::ivec2& origin, const y::ivec2& size, std::int32_t z_index = 0);
  ~Panel() override {}

  void set_origin(const y::ivec2& origin);
  void set_size(const y::ivec2& size);
  void set_visible(bool visible);

  const y::ivec2& get_origin() const;
  const y::ivec2& get_size() const;
  bool is_visible() const;

  virtual bool event(const sf::Event& e) = 0;
  virtual void update() = 0;
  virtual void draw(RenderUtil& util) const = 0;

  struct order {
    bool operator()(Panel* a, Panel* b) const;
  };

private:

  bool _visible;

  y::ivec2 _origin;
  y::ivec2 _size;
  std::int32_t _z_index;

};

// A set of panels making up a complete UI layer.
class PanelUi {
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

  typedef std::set<Panel*, Panel::order> panel_set;
  panel_set _panels;
  panel_set _mouse_over;

};

// Interface to an entry in the ModalStack.
class Modal : public Draggable {
public:

  Modal();
  ~Modal() override {}

  // Virtual interface.
  virtual void event(const sf::Event& e) = 0;
  virtual void update() = 0;
  virtual void draw() const = 0;

  // Push a new mode onto the top of the enclosing stack.
  void push(std::unique_ptr<Modal> modal);

  // Checks if there are any elements on the stack above this one.
  bool has_next() const;

  // By default, the stack will be drawn from bottom to top. Call this to
  // draw all higher elements early, in order to add some final behaviour.
  void draw_next() const;

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
  void clear_drawn_next() const;
  bool has_drawn_next() const;

  ModalStack* _stack;
  mutable bool _has_drawn_next;
  bool _end;

  UndoStack _undo_stack;
  PanelUi _panel_ui;

};

struct RunTiming {
  float target_updates_per_second;
  float target_draws_per_second;

  std::size_t us_per_update_inst;
  std::size_t us_per_update_avg;
  std::size_t us_per_draw_inst;
  std::size_t us_per_draw_avg;
  std::size_t us_per_frame_inst;
  std::size_t us_per_frame_avg;

  std::size_t updates_this_cycle;
  std::size_t draws_this_cycle;
};

// A stack of modes. The topmost mode receives all events and controls the
// program flow. Modes are rendered from back to front.
class ModalStack {
public:

  void push(std::unique_ptr<Modal> modal);
  bool empty() const;

  // Run until the stack is empty. Push a mode onto the stack first. If fps
  // is zero, no frame-compensation occurs. Stores timing information in the
  // RunTiming structure.
  void run(Window& window, RunTiming& run_timing);

private:

  friend class Modal;
  bool has_next(const Modal& modal) const;
  void draw_next(const Modal& modal) const;

  void event(const sf::Event& e, std::size_t index);
  void update();
  void draw(std::size_t index) const;
  bool clear_ended();

  typedef std::unique_ptr<Modal> element;
  typedef std::vector<element> stack;
  stack _stack;

};

#endif
