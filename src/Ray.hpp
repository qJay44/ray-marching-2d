#pragma once

#include <list>
#include <vector>

#include "OCL_RayMarching.hpp"

class Ray : public sf::Drawable {
public:
  Ray(sf::Vector2f origin, size_t maxMarches, int mode = 0);

  void setMode(int mode);

  void update(const sf::Vector2i& mousePos);
  void march(const std::vector<sf::CircleShape>& circles, const std::vector<sf::RectangleShape>& rects);
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
  sf::Vector2f origin;
  size_t maxMarches;
  int mode;
  OCL_RayMarching ocl;

  sf::Vector2f direction;
  float length;

  sf::CircleShape circleBase;
  sf::VertexArray line{sf::PrimitiveType::Lines, 2};
  std::list<sf::CircleShape> rayCircles;

  sf::Texture oclTexture;

private:
  float signedDstToCircle(const sf::Vector2f& p, const sf::Vector2f& center, float r) const;
  float signedDstToRectangle(const sf::Vector2f& p, const sf::Vector2f& center, const sf::Vector2f& size) const;
};

