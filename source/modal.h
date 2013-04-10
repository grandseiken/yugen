#ifndef MODAL_H
#define MODAL_H

#include "common.h"

namespace sf {
  class Event;
}
class ModalStack;
class Window;

// Interface to an entry in the ModalStack.
class Modal {
public:

  Modal();
  virtual ~Modal() {}

  virtual void event(const sf::Event& e) = 0;
  virtual void update() = 0;
  virtual void draw() const = 0;

  void push(y::unique<Modal> modal);
  void end();
  bool is_ended() const;

private:

  friend class ModalStack;
  void set_stack(ModalStack& stack);

  ModalStack* _stack;
  bool _end;

};

// A stack of modes. The topmost mode receives all events and control the
// program state. Modes are rendered from back to front.
class ModalStack {
public:

  void push(y::unique<Modal> modal);
  bool empty() const;

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
