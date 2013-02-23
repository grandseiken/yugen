#include "window.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>

Resolution::Resolution()
  : width(0)
  , height(0)
  , bpp(0)
{
}

Resolution::~Resolution()
{
}

Resolution::Resolution(const Resolution& r)
  : width(r.width), height(r.height), bpp(r.bpp)
{
}

Resolution& Resolution::operator=(const Resolution& r)
{
  width = r.width;
  height = r.height;
  bpp = r.bpp;
  return *this;
}

bool Resolution::operator==(const Resolution& r) const
{
  return width == r.width && height == r.height && bpp == r.bpp;
}

bool Resolution::operator!=(const Resolution& r) const
{
  return !operator==(r);
}

Window::Window(const y::string& title, y::size default_bpp,
               y::size default_width, y::size default_height,
               bool default_fullscreen, bool skip_choice)
  : _window(y::null)
{
  Resolution desktop;
  get_desktop_mode(desktop);

  y::vector<Resolution> supported_modes;
  get_supported_modes(supported_modes, 0);

  _resolution.width = default_width;
  _resolution.height = default_height;
  _resolution.bpp = default_bpp;
  if (!default_width || !default_height) {
    _resolution.width = 640;
    _resolution.height = 480;
  }

  // Let the user choose the resolution.
  if (!skip_choice) { 
    y::size i = 0;
    for (const Resolution& r : supported_modes) {
      std::cout << i++ << ") " <<
          r.width << "x" << r.height << " " << r.bpp << "bpp";
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

    if (!supported_modes.empty()) {
      y::size input;
      std::cin >> input;
      if (input < supported_modes.size()) {
        _resolution = supported_modes[input];
      }
      else {
        default_fullscreen = false;
      }
    }
  }

  // In fullscreen mode, find the closest mode to the given aspect ratio.
  if (default_fullscreen) {
    float aspect_ratio = _resolution.width / _resolution.height;
    float best_distance = 1000.f;
    y::size best_match = 0;
    Resolution best_resolution;

    for (const Resolution& r : supported_modes) {
      float distance = abs(aspect_ratio - r.width / r.height);
      y::size match = (r == desktop) + (r.bpp == _resolution.bpp);

      bool best_so_far =
          distance < best_distance ||
          (distance == best_distance && match > best_match) ||
          (distance == best_distance && match == best_match &&
           r.width > best_resolution.width);
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

  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 2;
  settings.minorVersion = 1;

  _window = std::move(y::unique<sf::Window>(new sf::RenderWindow(
      sf::VideoMode(_resolution.width, _resolution.height, _resolution.bpp),
      title, default_fullscreen ? sf::Style::Fullscreen : sf::Style::Default,
      settings)));
  _window->setVerticalSyncEnabled(true);
  _window->setActive();
}

Window::~Window()
{
}

const Resolution& Window::get_mode() const
{
  return _resolution;
}

bool Window::poll_event(sf::Event& output)
{
  while (_window->pollEvent(output)) {
    if (output.type == sf::Event::Resized) {
      glViewport(0, 0, output.size.width, output.size.height);
      _resolution.width = output.size.width;
      _resolution.height = output.size.height;
      continue;
    }
    return true;
  }
  return false;
}

void Window::display() const
{
  _window->display();
}

void Window::get_supported_modes(y::vector<Resolution>& output, y::size bpp)
{
  const y::vector<sf::VideoMode>& modes = sf::VideoMode::getFullscreenModes();
  for (const sf::VideoMode& mode : modes) {
    if (bpp && mode.bitsPerPixel != bpp) {
      continue;
    }
    Resolution r;
    r.width = mode.width;
    r.height = mode.height;
    r.bpp = mode.bitsPerPixel;
    output.push_back(r);
  }
}

void Window::get_desktop_mode(Resolution& output)
{
  sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  output.width = desktop.width;
  output.height = desktop.height;
  output.bpp = desktop.bitsPerPixel;
}
