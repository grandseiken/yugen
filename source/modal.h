#ifndef MODAL_H
#define MODAL_H

#include "common.h"

namespace sf {
  class Event;
}
class ModalStack;
class Window;

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
