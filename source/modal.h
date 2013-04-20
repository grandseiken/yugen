#ifndef MODAL_H
#define MODAL_H

#include "common.h"

namespace sf {
  class Event;
}
class ModalStack;
class Window;

// Interface to an entry in the ModalStack.
class Modal : public y::no_copy {
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

private:

  friend class ModalStack;
  void set_stack(ModalStack& stack);

  ModalStack* _stack;
  bool _end;

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

  void event(const sf::Event& e);
  void update();
  void draw() const;
  bool clear_ended();

  typedef y::unique<Modal> Element;
  typedef y::vector<Element> Stack;
  Stack _stack;

};

#endif
