#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"

struct Resolution {
  Resolution();
  ~Resolution();

  Resolution(const Resolution& r);
  Resolution& operator=(const Resolution& r);

  bool operator==(const Resolution& r) const;
  bool operator!=(const Resolution& r) const;

  y::size width;
  y::size height;
  y::size bpp;
};

namespace sf {
  class Window;
  class Event;
}

class Window {
public:

  Window(const y::string& title, y::size default_bpp,
         y::size default_width, y::size default_height,
         bool default_fullscreen, bool skip_choice);
  ~Window();

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
