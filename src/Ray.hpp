#pragma once

#include <list>

#include "OCL_SDF.hpp"

class Ray : public sf::Drawable {
public:
  Ray(sf::Vector2f origin, size_t maxMarches);

  void update(sf::Vector2i mousePos);
  void march(const u8* sdfPixels);
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
  sf::Vector2f origin;
  size_t maxMarches;

  sf::Vector2f direction;
  float length;

  sf::CircleShape circleBase;
  sf::VertexArray line{sf::PrimitiveType::Lines, 2};
  std::list<sf::CircleShape> rayCircles;
};

