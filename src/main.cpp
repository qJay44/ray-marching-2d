#include <cstdlib>
#include <format>
#include <direct.h>

#include "Ray.hpp"
#include "ShapeContainer.hpp"
#include "utils/utils.hpp"

int main() {
  // Assuming the executable is launching from its own directory
  _chdir("../../../src");

  // srand(static_cast<unsigned int>(time(nullptr)));
  sf::RenderWindow window = sf::RenderWindow(sf::VideoMode({WIDTH, HEIGHT}), "CMake SFML Project");
  window.setFramerateLimit(144);

  std::string fontPath = "res/fonts/monocraft/Monocraft.ttf";
  sf::Font font;
  if (!font.openFromFile(fontPath))
    error("Can't open font [{}]", fontPath);

  // Shapes setup
  int drawMode = 0;
  int numCircles = 3;
  int numRects = 3;
  ShapeContainer shapeContainer;
  shapeContainer.generate(numCircles, numRects);

  sf::RectangleShape wall({100, 700});
  wall.setPosition({500, 500});
  wall.setFillColor(sf::Color::Black);
  shapeContainer.rects.push_back(wall);

  // OpenCL related
  OCL_SDF ocl(WIDTH, HEIGHT);
  sf::Texture sdfTexture({WIDTH, HEIGHT});
  sf::Sprite sdfSprite(sdfTexture);

  // Ray march shader
  sf::Shader rmShader(fspath("rm.frag"), sf::Shader::Type::Fragment);
  sf::RectangleShape rmRect({WIDTH, HEIGHT});
  sf::RenderTexture shapesTexture({WIDTH, HEIGHT});
  sf::RenderTexture previousFrame({WIDTH, HEIGHT});
  sf::RenderTexture currentFrame({WIDTH, HEIGHT});
  sf::Texture blueNoise("res/tex/LDR_LLL1_0.png");
  rmShader.setUniform("u_baseTexture", shapesTexture.getTexture());
  rmShader.setUniform("u_sdfTexture", sdfTexture);
  rmShader.setUniform("u_blueNoiseTexture", blueNoise);
  rmShader.setUniform("u_resolution", sf::Glsl::Vec2{WIDTH, HEIGHT});
  rmShader.setUniform("u_raysPerPixel", 32);
  rmShader.setUniform("u_stepsPerRay", 32);
  rmShader.setUniform("u_epsilon", 0.001f);

  // Single ray
  Ray ray(sf::Vector2f{20.f, 20.f}, 64);

  // Loop related
  sf::Clock clock;
  sf::Vector2i mousePos;
  float titleTime = 0.f;
  float dt;

  shapesTexture.clear();
  shapesTexture.draw(shapeContainer);
  shapesTexture.display();

  while (window.isOpen()) {
    // ----- Events ----------------------------------- //

    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        switch (keyPressed->scancode) {
          case sf::Keyboard::Scancode::Q:
            window.close();
            break;
          case sf::Keyboard::Scancode::R:
            shapeContainer.generate(numCircles, numRects);
            break;
          case sf::Keyboard::Scancode::S:
            shapeContainer.showShapes = !shapeContainer.showShapes;
            break;
          case sf::Keyboard::Scancode::Num1:
            drawMode = 0;
            break;
          case sf::Keyboard::Scancode::Num2:
            drawMode = 1;
            break;
          case sf::Keyboard::Scancode::Num3:
            drawMode = 2;
            break;
          default:
            break;
        };
      } else if (const auto* mouseBtnPressed = event->getIf<sf::Event::MouseButtonReleased>()) {
        switch (mouseBtnPressed->button) {
          case sf::Mouse::Button::Left:
            shapeContainer.update(mousePos, false);
            break;
          default:
            break;
        }
      }
    }

    // ----- Update meta ------------------------------ //

    dt = clock.restart().asSeconds();
    mousePos = sf::Mouse::getPosition(window);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
      shapeContainer.update(mousePos, true);

      shapesTexture.clear();
      shapesTexture.draw(shapeContainer);
      shapesTexture.display();

      rmShader.setUniform("u_baseTexture", shapesTexture.getTexture());
    }

    if (titleTime > 0.3f) {
      window.setTitle(std::format("FPS: {}, {:.2f} ms", static_cast<int>(1.f / dt), dt * 1000.f));
      titleTime = 0.f;
    } else {
      titleTime += dt;
    }

    // ----- Update objects --------------------------- //

    // TODO: Update only if atleast one shape were changed (TODO: Update only the updated shape?)
    ocl.updateCirclesBuffer(shapeContainer.circles);
    ocl.updateRectsBuffer(shapeContainer.rects);
    ocl.run();

    const u8* sdfPixels = ocl.getPixels();

    ray.update(mousePos);
    ray.march(sdfPixels);

    // ----- Draw ------------------------------------- //

    window.clear({10, 10, 10, 255});

    switch (drawMode) {
      case 0: {
        window.draw(ray);
        window.draw(shapeContainer);
        window.display();
        break;
      }
      case 1: {
        sdfTexture.update(sdfPixels);
        sdfSprite = sf::Sprite(sdfTexture);
        window.draw(sdfSprite);
        window.draw(shapeContainer);
        window.display();
        break;
      }
      case 2: {
        sdfTexture.update(sdfPixels);

        currentFrame.clear();
        currentFrame.draw(rmRect, &rmShader);
        currentFrame.display();

        const sf::Sprite currentFrameSprite(currentFrame.getTexture());

        window.draw(currentFrameSprite);
        window.display();

        previousFrame.clear();
        previousFrame.draw(currentFrameSprite);
        previousFrame.display();

        rmShader.setUniform("u_baseTexture", previousFrame.getTexture());
        break;
      }
    }
  }
}

