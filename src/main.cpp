#include <cstdlib>
#include <format>
#include <direct.h>

#include "Ray.hpp"
#include "ShapeContainer.hpp"
#include "utils/utils.hpp"

int main() {
  // Assuming the executable is launching from its own directory
  _chdir("../../../src");

  srand(static_cast<unsigned int>(time(nullptr)));
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
  int numWalls = 2;
  ShapeContainer shapeContainer;
  shapeContainer.generate(numCircles, numRects, numWalls);

  // OpenCL related
  OCL_SDF ocl(WIDTH, HEIGHT);
  sf::Texture sdfTexture({WIDTH, HEIGHT});
  sf::Sprite sdfSprite(sdfTexture);

  // ----- Ray march shader ------------------------- //

  sf::Shader rmShader(fspath("rm.frag"), sf::Shader::Type::Fragment);
  sf::RectangleShape rmRect({WIDTH, HEIGHT});
  sf::RenderTexture shapesTexture({WIDTH, HEIGHT});
  sf::RenderTexture previousFrame({WIDTH, HEIGHT});
  sf::RenderTexture currentFrame({WIDTH, HEIGHT});
  sf::Texture blueNoise("res/tex/LDR_LLL1_0.png");

  int raysPerPixel = 32;
  int stepsPerRay = 32;
  float epsilon = 0.001f;
  rmShader.setUniform("u_baseTexture", shapesTexture.getTexture());
  rmShader.setUniform("u_sdfTexture", sdfTexture);
  rmShader.setUniform("u_blueNoiseTexture", blueNoise);
  rmShader.setUniform("u_resolution", sf::Glsl::Vec2{WIDTH, HEIGHT});
  rmShader.setUniform("u_raysPerPixel", raysPerPixel);
  rmShader.setUniform("u_stepsPerRay", stepsPerRay);
  rmShader.setUniform("u_epsilon", epsilon);

  // ----- Texts ------------------------------------ //

  sf::Text baseText(font, "baseText", 12);
  baseText.setOutlineThickness(1.f);
  baseText.setOutlineColor(sf::Color::Black);

  sf::Vector2f textOffset = baseText.getGlobalBounds().size;
  textOffset.x = 0.f;
  textOffset.y += 2.f;

  sf::Text raysPerPixelText(baseText);
  raysPerPixelText.setPosition(sf::Vector2f{5.f, 5.f});
  raysPerPixelText.setString(std::format("raysPerPixel = {}", raysPerPixel));

  sf::Text stepsPerRayText(baseText);
  stepsPerRayText.setPosition(raysPerPixelText.getPosition() + textOffset);
  stepsPerRayText.setString(std::format("stepsPerRay = {}", stepsPerRay));

  sf::Text epsilonText(baseText);
  epsilonText.setPosition(stepsPerRayText.getPosition() + textOffset);
  epsilonText.setString(std::format("epsilon = {}", epsilon));

  // ------------------------------------------------ //

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
            shapeContainer.generate(numCircles, numRects, numWalls);
            shapesTexture.clear();
            shapesTexture.draw(shapeContainer);
            shapesTexture.display();
            rmShader.setUniform("u_baseTexture", shapesTexture.getTexture());
            break;
          case sf::Keyboard::Scancode::C:
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
          case sf::Keyboard::Scancode::W:
            raysPerPixel = std::min(raysPerPixel * 2, 1024);
            rmShader.setUniform("u_raysPerPixel", raysPerPixel);
            raysPerPixelText.setString(std::format("raysPerPixel = {}", raysPerPixel));
            break;
          case sf::Keyboard::Scancode::S:
            raysPerPixel = std::max(raysPerPixel / 2, 1);
            rmShader.setUniform("u_raysPerPixel", raysPerPixel);
            raysPerPixelText.setString(std::format("raysPerPixel = {}", raysPerPixel));
            break;
          case sf::Keyboard::Scancode::A:
            stepsPerRay = std::max(stepsPerRay / 2, 1);
            rmShader.setUniform("u_stepsPerRay", stepsPerRay);
            stepsPerRayText.setString(std::format("stepsPerRay = {}", stepsPerRay));
            break;
          case sf::Keyboard::Scancode::D:
            stepsPerRay = std::min(stepsPerRay * 2, 1024);
            rmShader.setUniform("u_stepsPerRay", stepsPerRay);
            stepsPerRayText.setString(std::format("stepsPerRay = {}", stepsPerRay));
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
      } else if (const auto* scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
        if (scrolled->delta < 0.f)
          epsilon *= 0.1f;
        else
          epsilon *= 10.f;

        rmShader.setUniform("u_epsilon", epsilon);
        epsilonText.setString(std::format("epsilon = {}", epsilon));
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
        window.draw(raysPerPixelText);
        window.draw(stepsPerRayText);
        window.draw(epsilonText);
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

