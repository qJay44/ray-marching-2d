#include <cstdlib>
#include <format>
#include <direct.h>

#include "Ray.hpp"
#include "ShapeContainer.hpp"
#include "utils/utils.hpp"

int main() {
  // Assuming the executable is launching from its own directory
  _chdir("../../../src");

  srand(static_cast<unsigned int>(time(nullptr)));
  sf::RenderWindow window = sf::RenderWindow(sf::VideoMode({WIDTH, HEIGHT}), "CMake SFML Project");
  window.setFramerateLimit(144);

  std::string fontPath = "fonts/monocraft/Monocraft.ttf";
  sf::Font font;
  if (!font.openFromFile(fontPath))
    error("Can't open font [{}]", fontPath);

  int numCircles = 3;
  int numRects = 3;
  ShapeContainer shaperContainer;
  shaperContainer.generate(numCircles, numRects);

  Ray ray(sf::Vector2f{20.f, 20.f}, 64);

  sf::Clock clock;
  sf::Vector2i mousePos;

  float titleTime = 0.f;
  float dt;
  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        switch (keyPressed->scancode) {
          case sf::Keyboard::Scancode::Q:
            window.close();
            break;
          case sf::Keyboard::Scancode::R:
            shaperContainer.generate(numCircles, numRects);
            break;
          case sf::Keyboard::Scancode::S:
            shaperContainer.showShapes = !shaperContainer.showShapes;
            break;
          case sf::Keyboard::Scancode::Num1:
            ray.setMode(0);
            break;
          case sf::Keyboard::Scancode::Num2:
            ray.setMode(1);
            break;
          default:
            break;
        };
      }
    }

    dt = clock.restart().asSeconds();
    mousePos = sf::Mouse::getPosition(window);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
      shaperContainer.update(mousePos, true);
    else
      shaperContainer.update(mousePos, false);

    if (titleTime > 0.3f) {
      window.setTitle(std::format("FPS: {}, {:.2f} ms", static_cast<int>(1.f / dt), dt * 1000.f));
      titleTime = 0.f;
    } else {
      titleTime += dt;
    }

    ray.update(mousePos);
    ray.march(shaperContainer);

    window.clear({10, 10, 10, 255});

    window.draw(ray);
    window.draw(shaperContainer);

    window.display();
  }
}

