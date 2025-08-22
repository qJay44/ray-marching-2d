#include "Ray.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

Ray::Ray(sf::Vector2f origin, size_t maxMarches, int mode)
  : origin(origin), maxMarches(maxMarches), mode(mode), ocl(WIDTH, HEIGHT), sdfTexture({WIDTH, HEIGHT}), sdfSprite(sdfTexture) {
  circleBase.setFillColor({30, 30, 30, 40});
  circleBase.setOutlineThickness(2.f);
  circleBase.setOutlineColor({90, 90, 90, 255});
  circleBase.setPointCount(100);
  circleBase.setRadius(1.f);
  circleBase.setOrigin({1.f, 1.f});
  circleBase.setPosition(origin);
};

void Ray::setMode(int mode) {
  this->mode = mode;
}

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

void Ray::march(const ShapeContainer& shapeContainer) {
  // TODO: Update only if atleast one shape were changed (TODO: Update only the updated shape?)
  ocl.updateCirclesBuffer(shapeContainer.circles);
  ocl.updateRectsBuffer(shapeContainer.rects);
  ocl.run();

  const float* sdfPixels = ocl.getPixels();

  switch (mode) {
    case 0: {
      sf::Vector2f currentOrigin = origin;
      float currentLength = 0.f;

      for (size_t i = 0; currentLength < length && i < maxMarches; i++) {
        // The update function clamps to [0, WIDTH], so its safe to cast to unsigned types
        size_t px = static_cast<size_t>(currentOrigin.x);
        size_t py = static_cast<size_t>(currentOrigin.y);
        size_t pixIdx = py * WIDTH + px;
        float dstToScene = sdfPixels[pixIdx] * WIDTH;

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
      break;
    }
    case 1: {
      // sdfTexture.update(sdfPixels);
      // sdfSprite = sf::Sprite(sdfTexture);
      break;
    }
    default:
      error("[Ray::march] Unhandled mode [{}]", mode);
  }
}

void Ray::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  switch (mode) {
    case 0: {
      target.draw(line);
      states.blendMode = sf::BlendAdd;

      for (const sf::CircleShape& circle : rayCircles)
        target.draw(circle, states);

      break;
    }
    case 1: {
      target.draw(sdfSprite);
      break;
    }
    default:
      error("[Ray::draw] Unhandled mode [{}]", mode);
  }
}

float Ray::signedDstToCircle(const sf::Vector2f& p, const sf::Vector2f& center, float r) const {
  return (center - p).length() - r;
}

float Ray::signedDstToRectangle(const sf::Vector2f& p, const sf::Vector2f& center, const sf::Vector2f& size) const {
  sf::Vector2f v = p - center;
  sf::Vector2f offset = sf::Vector2f{abs(v.x), abs(v.y)} - size;
  float unsignedDst  = sf::Vector2f{std::max(offset.x, 0.f), std::max(offset.y, 0.f)}.length();

  sf::Vector2f dstInsideBoxV = sf::Vector2f{std::min(offset.x, 0.f), std::min(offset.y, 0.f)};
  float dstInsideBox = std::max(dstInsideBoxV.x, dstInsideBoxV.y);

  return unsignedDst + dstInsideBox;
}
