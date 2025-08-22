#pragma once

#include <list>

#include "OCL_SDF.hpp"
#include "ShapeContainer.hpp"

class Ray : public sf::Drawable {
public:
  Ray(sf::Vector2f origin, size_t maxMarches, int mode = 0);

  void setMode(int mode);

  void update(sf::Vector2i mousePos);
  void march(const ShapeContainer& shapeContainer);
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
  sf::Vector2f origin;
  size_t maxMarches;
  int mode;
  OCL_SDF ocl;

  sf::Texture sdfTexture;
  sf::Sprite sdfSprite;

  sf::Vector2f direction;
  float length;

  sf::CircleShape circleBase;
  sf::VertexArray line{sf::PrimitiveType::Lines, 2};
  std::list<sf::CircleShape> rayCircles;

private:
  float signedDstToCircle(const sf::Vector2f& p, const sf::Vector2f& center, float r) const;
  float signedDstToRectangle(const sf::Vector2f& p, const sf::Vector2f& center, const sf::Vector2f& size) const;
};

