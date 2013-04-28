#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"
#include "vector.h"

struct Resolution {
  Resolution();

  bool operator==(const Resolution& r) const;
  bool operator!=(const Resolution& r) const;

  y::ivec2 size;
  y::size bpp;
};

namespace sf {
  class Event;
  class Window;
}

class Window : public y::no_copy {
public:

  static const y::size framerate = 60;

  // In fullscreen, window will use closest mode to parameters by default,
  // unless choice is allowed. In windowed, parameters are the defaults but
  // the window can be resized.
  Window(const y::string& title, y::size default_bpp,
         const y::ivec2& default_size,
         bool default_fullscreen, bool skip_choice);

  const Resolution& get_mode() const;

  bool poll_event(sf::Event& output);
  void set_active() const;
  void display() const;

  static void get_supported_modes(y::vector<Resolution>& output, y::size bpp);
  static void get_desktop_mode(Resolution& output);

private:

  y::unique<sf::Window> _window;
  Resolution _resolution;

};

#endif
