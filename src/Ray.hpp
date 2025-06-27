#pragma once

#include <algorithm>
#include <list>
#include <vector>

class Ray : public sf::Drawable {
public:
  Ray(sf::Vector2f origin) : origin(origin) {
    circleBase.setFillColor({30, 30, 30, 40});
    circleBase.setOutlineThickness(2.f);
    circleBase.setOutlineColor({90, 90, 90, 255});
    circleBase.setPointCount(100);
    circleBase.setRadius(1.f);
    circleBase.setOrigin({1.f, 1.f});
    circleBase.setPosition(origin);
  };

  void update(const sf::Vector2i& mousePos) {
    sf::Vector2f v = (sf::Vector2f(mousePos) - origin);
    length = v.length();
    direction = v / length;

    line[0].position = origin;
    line[1].position = origin + direction * length;
    rayCircles.clear();
  }

  void march(const std::vector<sf::RectangleShape>& rects, const std::vector<sf::CircleShape>& circles) {
    sf::Vector2f currentOrigin = origin;
    float currentLength = 0.f;
    float dstToScene = length;

    for (size_t i = 0; dstToScene > 1.f && currentLength < length; i++) {
      for (const sf::RectangleShape& rect : rects) {
        sf::Vector2f sizeFromCenter = rect.getGeometricCenter();
        sf::Vector2f centerGlobal = rect.getPosition() + sizeFromCenter;
        dstToScene = std::min(dstToScene, signedDstToRectangle(currentOrigin, centerGlobal, sizeFromCenter));
      }

      for (const sf::CircleShape& circle : circles) {
        sf::Vector2f center = circle.getPosition();
        dstToScene = std::min(dstToScene, signedDstToCircle(currentOrigin, center, circle.getRadius()));
      }

      sf::CircleShape newCircle(circleBase);
      newCircle.setRadius(dstToScene);
      newCircle.setPosition(currentOrigin);
      newCircle.setOrigin(sf::Vector2f{dstToScene, dstToScene});
      rayCircles.push_back(newCircle);

      currentOrigin += direction * dstToScene;
      currentLength += dstToScene;
    }

    line[1].position = origin + direction * currentLength;
  }

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
    target.draw(line);
    states.blendMode = sf::BlendAdd;

    for (const sf::CircleShape& circle : rayCircles)
      target.draw(circle, states);
  }

private:
  sf::Vector2f origin;
  sf::Vector2f direction;
  float length;

  sf::CircleShape circleBase;
  sf::VertexArray line{sf::PrimitiveType::Lines, 2};
  std::list<sf::CircleShape> rayCircles;

private:
  float signedDstToRectangle(const sf::Vector2f& p, const sf::Vector2f& center, const sf::Vector2f& size) const {
    sf::Vector2f v = p - center;
    sf::Vector2f offset = sf::Vector2f{abs(v.x), abs(v.y)} - size;
    float unsignedDst  = sf::Vector2f{std::max(offset.x, 0.f), std::max(offset.y, 0.f)}.length();

    sf::Vector2f dstInsideBoxV = sf::Vector2f{std::min(offset.x, 0.f), std::min(offset.y, 0.f)};
    float dstInsideBox = std::max(dstInsideBoxV.x, dstInsideBoxV.y);

    return unsignedDst + dstInsideBox;
  }

  float signedDstToCircle(const sf::Vector2f& p, const sf::Vector2f& center, float r) const {
    return (center - p).length() - r;
  }
};

