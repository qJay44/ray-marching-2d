#include <cstdlib>
#include <vector>

#include "Ray.hpp"

int randBetween(int min, int max) {
  return rand() % (max - min + 1) + min;
}

sf::Vector2f randPos() {
  return sf::Vector2f(rand() % WIDTH, rand() % HEIGHT);
}

sf::Color randColor() {
  return sf::Color(rand() % 256, rand() % 256, rand() % 256);
}

int main() {
  srand(static_cast<unsigned int>(time(nullptr)));
  sf::RenderWindow window = sf::RenderWindow(sf::VideoMode({WIDTH, HEIGHT}), "CMake SFML Project");
  window.setFramerateLimit(144);

  std::vector<sf::RectangleShape> rects;
  std::vector<sf::CircleShape> circles;

  const auto generateShapes = [&rects, &circles]() {
    rects.clear();
    for (size_t i = 0; i < 5; i++) {
      sf::Vector2f size(randBetween(10, 50), randBetween(10, 50));
      sf::Vector2f pos = randPos();
      sf::RectangleShape rect(size);
      rect.setPosition(pos);
      rect.setFillColor(randColor());
      rects.push_back(rect);
    }

    circles.clear();
    for (size_t i = 0; i < 5; i++) {
      float radius = randBetween(10, 50);
      sf::Vector2f pos = randPos();
      sf::CircleShape circle(100);
      circle.setRadius(radius);
      circle.setOrigin({radius, radius});
      circle.setPosition(pos);
      circle.setFillColor(randColor());
      circles.push_back(circle);
    }
  };

  Ray ray{sf::Vector2f{20.f, 20.f}};

  generateShapes();

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
            generateShapes();
            break;
          default:
            break;
        };
      }
    }

    ray.update(sf::Mouse::getPosition(window));
    ray.march(rects, circles);

    window.clear({10, 10, 10, 255});

    for (const sf::RectangleShape& rect : rects) window.draw(rect);
    for (const sf::CircleShape& circle : circles) window.draw(circle);

    window.draw(ray);

    window.display();
  }
}

