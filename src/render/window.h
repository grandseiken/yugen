#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "../vec.h"
#include <memory>
#include <string>
#include <vector>

struct Resolution {
  Resolution();

  bool operator==(const Resolution& r) const;
  bool operator!=(const Resolution& r) const;

  y::ivec2 size;
  std::size_t bpp;
};

namespace sf {
  class Event;
  class Window;
}

class Window {
public:

  static const std::size_t framerate = 60;

  // In fullscreen, window will use closest mode to parameters by default,
  // unless choice is allowed. In windowed, parameters are the defaults but
  // the window can be resized.
  Window(const std::string& title, std::size_t default_bpp,
         const y::ivec2& default_size,
         bool default_fullscreen, bool skip_choice);

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  const Resolution& get_mode() const;

  bool poll_event(sf::Event& output);
  void set_active() const;
  void set_key_repeat(bool repeat) const;
  void display() const;

  static void get_supported_modes(
      std::vector<Resolution>& output, std::size_t bpp);
  static void get_desktop_mode(Resolution& output);

private:

  std::unique_ptr<sf::Window> _window;
  Resolution _resolution;

};

#endif
