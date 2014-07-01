#include "window.h"
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

Resolution::Resolution()
  : size{0, 0}
  , bpp(0)
{
}

bool Resolution::operator==(const Resolution& r) const
{
  return size == r.size && bpp == r.bpp;
}

bool Resolution::operator!=(const Resolution& r) const
{
  return !operator==(r);
}

Window::Window(const std::string& title, std::size_t default_bpp,
               const y::ivec2& default_size,
               bool default_fullscreen, bool skip_choice)
  : _window(nullptr)
{
  Resolution desktop;
  get_desktop_mode(desktop);

  std::vector<Resolution> supported_modes;
  get_supported_modes(supported_modes, 0);

  _resolution.size = default_size;
  _resolution.bpp = default_bpp;

  // Let the user choose the resolution.
  if (!skip_choice) {
    std::size_t i = 0;
    for (const Resolution& r : supported_modes) {
      std::cout << i++ << ") " <<
          r.size[xx] << "x" << r.size[yy] << " " << r.bpp << "bpp";
      if (desktop == r) {
        std::cout << " (*)";
      }

      if (i % 4) {
        std::cout << '\t';
      }
      else {
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;

    if (!supported_modes.empty()) {
      std::size_t input;
      std::cin >> input;
      if (input < supported_modes.size()) {
        _resolution = supported_modes[input];
        default_fullscreen = true;
      }
      else {
        default_fullscreen = false;
      }
    }
  }

  // In fullscreen mode, find the closest mode to the given aspect ratio.
  if (default_fullscreen) {
    float aspect_ratio = _resolution.size[yy] ?
        float(_resolution.size[xx]) / _resolution.size[yy] : 0.f;
    float best_distance = 1000.f;
    std::size_t best_match = 0;
    Resolution best_resolution;

    for (const Resolution& r : supported_modes) {
      float distance = fabs(aspect_ratio - float(r.size[xx]) / r.size[yy]);
      std::size_t match = (r == desktop) + (r.bpp == _resolution.bpp);

      bool best_so_far = false;
      if (_resolution.size[yy]) {
        // Heuristic for closest mode.
        best_so_far =
            distance < best_distance ||
            (distance == best_distance && match > best_match) ||
            (distance == best_distance && match == best_match &&
             r.size[xx] > best_resolution.size[xx]);
      }
      else {
        best_so_far = match > best_match ||
            (match == best_match && r.size[xx] > best_resolution.size[xx]);
      }

      if (best_so_far) {
        best_distance = distance;
        best_match = match;
        best_resolution = r;
      }
    }

    if (supported_modes.empty()) {
      default_fullscreen = false;
    }
    else {
      _resolution = best_resolution;
    }
  }

  if (!(_resolution.size > y::ivec2())) {
    _resolution.size = desktop.size / 2;
  }

  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 2;
  settings.minorVersion = 1;

  _window.reset(new sf::RenderWindow(
      sf::VideoMode(_resolution.size[xx], _resolution.size[yy],
                    _resolution.bpp),
      title, default_fullscreen ? sf::Style::Fullscreen : sf::Style::Default,
      settings));
  _window->setFramerateLimit(0);
  _window->setVerticalSyncEnabled(true);
  _window->setActive();
}

const Resolution& Window::get_mode() const
{
  return _resolution;
}

bool Window::poll_event(sf::Event& output)
{
  while (_window->pollEvent(output)) {
    if (output.type == sf::Event::Resized) {
      _resolution.size = {std::int32_t(output.size.width),
                          std::int32_t(output.size.height)};
      continue;
    }
    return true;
  }
  return false;
}

void Window::set_active() const
{
  _window->setActive();
}

void Window::set_key_repeat(bool repeat) const
{
  _window->setKeyRepeatEnabled(repeat);
}

void Window::display() const
{
  _window->display();
}

void Window::get_supported_modes(std::vector<Resolution>& output, std::size_t bpp)
{
  const std::vector<sf::VideoMode>& modes = sf::VideoMode::getFullscreenModes();
  for (const sf::VideoMode& mode : modes) {
    if (bpp && mode.bitsPerPixel != bpp) {
      continue;
    }
    Resolution r;
    r.size = {std::int32_t(mode.width), std::int32_t(mode.height)};
    r.bpp = mode.bitsPerPixel;
    output.emplace_back(r);
  }
}

void Window::get_desktop_mode(Resolution& output)
{
  sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  output.size = {std::int32_t(desktop.width), std::int32_t(desktop.height)};
  output.bpp = desktop.bitsPerPixel;
}
