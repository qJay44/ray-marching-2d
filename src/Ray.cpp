#include "Ray.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "defines.hpp"

Ray::Ray(sf::Vector2f origin, size_t maxMarches)
  : origin(origin), maxMarches(maxMarches) {
  circleBase.setFillColor({30, 30, 30, 40});
  circleBase.setOutlineThickness(2.f);
  circleBase.setOutlineColor({90, 90, 90, 255});
  circleBase.setPointCount(100);
  circleBase.setRadius(1.f);
  circleBase.setOrigin({1.f, 1.f});
  circleBase.setPosition(origin);
};

void Ray::update(sf::Vector2i mousePos) {
  sf::Vector2f mousePosClamped(
    std::clamp(mousePos.x, 0, (int)WIDTH),
    std::clamp(mousePos.y, 0, (int)HEIGHT)
  );

  sf::Vector2f v = mousePosClamped - origin;
  length = v.length();
  direction = v / length;

  line[0].position = origin;
  line[1].position = origin + direction * length;
  rayCircles.clear();
}

void Ray::march(const u8* sdfPixels) {
  sf::Vector2f currentOrigin = origin;
  float currentLength = 0.f;

  for (size_t i = 0; currentLength < length && i < maxMarches; i++) {
    size_t px = static_cast<size_t>(currentOrigin.x);
    size_t py = static_cast<size_t>(currentOrigin.y);
    size_t pixIdx = (py * WIDTH + px) * 4; // [R, G, B, A, R, G, B, A, ...]
    float dstToScene = sdfPixels[pixIdx] / 255.f * WIDTH;

    if (dstToScene < 10.f)
      break;

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

void Ray::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  target.draw(line);
  states.blendMode = sf::BlendAdd;

  for (const sf::CircleShape& circle : rayCircles)
    target.draw(circle, states);
}

