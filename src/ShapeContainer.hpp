#pragma once

struct ShapeContainer : public sf::Drawable {
  std::vector<sf::RectangleShape> rects;
  std::vector<sf::CircleShape> circles;
  bool showShapes = true;

  sf::Shape* holdingShape = nullptr;

  void generate(int numCircles, int numRects) {
    circles.clear();
    for (int i = 0; i < numCircles; i++) {
      float radius = randBetween(50, 100);
      sf::Vector2f pos = randPos();
      sf::CircleShape circle(100);
      circle.setRadius(radius);
      circle.setOrigin({radius, radius});
      circle.setPosition(pos);
      circle.setFillColor(randColor());
      circles.push_back(circle);
    }

    rects.clear();
    for (int i = 0; i < numRects; i++) {
      sf::Vector2f size(randBetween(50, 100), randBetween(50, 100));
      sf::Vector2f pos = randPos();
      sf::RectangleShape rect(size);
      rect.setPosition(pos);
      rect.setFillColor(randColor());
      rects.push_back(rect);
    }
  };

  void update(const sf::Vector2f& mousePos, bool hold) {
    if (!holdingShape && hold) {
      for (sf::CircleShape& circle : circles) {
        if (circle.getGlobalBounds().contains(sf::Vector2f(mousePos))) {
          holdingShape = &circle;
          break;
        }
      }

      for (sf::RectangleShape& rect : rects) {
        if (rect.getGlobalBounds().contains(sf::Vector2f(mousePos))) {
          holdingShape = &rect;
          break;
        }
      }
    } else {
      if (hold)
        holdingShape->setPosition(mousePos);
      else
        holdingShape = nullptr;
    }
  }

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
    if (showShapes) {
      for (const sf::CircleShape& circle : circles) target.draw(circle);
      for (const sf::RectangleShape& rect : rects)  target.draw(rect);
    }
  }

private:
  int randBetween(int min, int max) {
    return rand() % (max - min + 1) + min;
  }

  sf::Vector2f randPos() {
    return sf::Vector2f(rand() % WIDTH, rand() % HEIGHT);
  }

  sf::Color randColor() {
    return sf::Color(rand() % 256, rand() % 256, rand() % 256);
  }
};

