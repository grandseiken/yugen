#include <GL/glew.h>
#include <GL/glut.h>
#include <SFML/Graphics.hpp>
#include <iostream>

#include "common.h"

int main(int argc, char** argv)
{
  const y::vector<sf::VideoMode>& modes = sf::VideoMode::getFullscreenModes();
  sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  y::size i = 0;
  for (const sf::VideoMode& mode : modes) {
    std::cout << i++ << ") " << mode.width << "x" << mode.height << " " <<
        mode.bitsPerPixel << "bpp";
    if (desktop.width == mode.width && desktop.height == mode.height &&
        desktop.bitsPerPixel == mode.bitsPerPixel) {
      std::cout << " (*)";
    }
    std::cout << std::endl;
  }
  y::size input;
  std::cin >> input;
  sf::RenderWindow window(input >= modes.size() ? desktop : modes[input],
                          "Crunk Yugen", sf::Style::Fullscreen);
  return 0;
}
