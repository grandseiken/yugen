#include "modal.h"
#include "window.h"

#include <SFML/Window.hpp>

Modal::Modal()
  : _end(false)
{
}

void Modal::push(y::unique<Modal> modal)
{
  _stack->push(std::move(modal));
}

void Modal::end()
{
  _end = true;
}

bool Modal::is_ended() const
{
  return _end;
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
  _stack[_stack.size() - 1]->event(e);
}

void ModalStack::update()
{
  if (empty()) {
    return;
  }
  _stack[_stack.size() - 1]->update();
}

void ModalStack::draw() const
{
  if (empty()) {
    return;
  }
  _stack[_stack.size() - 1]->draw();
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
