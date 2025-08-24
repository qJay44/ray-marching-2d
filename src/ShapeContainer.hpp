#pragma once

#include <algorithm>

#include "defines.hpp"

struct ShapeContainer : public sf::Drawable {
  std::vector<sf::RectangleShape> rects;
  std::vector<sf::CircleShape> circles;
  bool showShapes = true;

  sf::Shape* holdingShape = nullptr;

  void generate(int numCircles, int numRects, int numWalls) {
    circles.clear();
    for (int i = 0; i < numCircles; i++) {
      float radius = randBetween(10, 80);
      sf::Vector2f pos = randPos();
      sf::CircleShape circle(100);

      circle.setRadius(radius);
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

    for (int i = 0; i < numWalls; i++) {
      // Vertical wall
      float width = randBetween(10, 30);
      float height = randBetween(500, 700);

      sf::Vector2f size(width, height);
      sf::Vector2f pos = randPos();

      // Make horizontal walls sometimes
      if (randBetween(0, 100) > 50) {
        size.x = height;
        size.y = width;
      }

      sf::RectangleShape rect(size);
      rect.setPosition(pos);
      rect.setFillColor(sf::Color::Black);
      rects.push_back(rect);
    }
  };

  void update(sf::Vector2i mousePos, bool hold) {
    sf::Vector2f mousePosClamped(
      std::clamp(mousePos.x, 0, (int)WIDTH),
      std::clamp(mousePos.y, 0, (int)HEIGHT)
    );

    if (!holdingShape && hold) {
      for (sf::CircleShape& circle : circles) {
        if (circle.getGlobalBounds().contains(mousePosClamped)) {
          holdingShape = &circle;
          sf::Vector2f shapePos = holdingShape->getPosition();
          sf::Vector2f distToMous = mousePosClamped - shapePos;
          holdingShape->setOrigin(distToMous);
          holdingShape->setPosition(shapePos + distToMous);
          break;
        }
      }

      for (sf::RectangleShape& rect : rects) {
        if (rect.getGlobalBounds().contains(mousePosClamped)) {
          holdingShape = &rect;
          sf::Vector2f shapePos = holdingShape->getPosition();
          sf::Vector2f distToMous = mousePosClamped - shapePos;
          holdingShape->setOrigin(distToMous);
          holdingShape->setPosition(shapePos + distToMous);
          break;
        }
      }
    } else {
      if (hold) {
        holdingShape->setPosition(mousePosClamped);
      } else {
        if (holdingShape) {
          sf::Vector2f currOrigin = holdingShape->getOrigin();
          holdingShape->setOrigin({0.f, 0.f});

          sf::Vector2f shapePos = holdingShape->getPosition();
          holdingShape->setPosition(shapePos - currOrigin);
        }
        holdingShape = nullptr;
      }
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

