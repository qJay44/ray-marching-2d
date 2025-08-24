#include <vector>

#include "CL/cl.h"
#include "utils/types.hpp"

class OCL_SDF {
public:
  OCL_SDF(size_t width, size_t height, bool printInfo = false);
  ~OCL_SDF();

  void updateCirclesBuffer(const std::vector<sf::CircleShape>& circles);
  void updateRectsBuffer(const std::vector<sf::RectangleShape>& rects);

  void run();

  [[nodiscard]]
  const u8* getPixels() const;

private:
  const size_t width, height;
  const size_t imageSize;
  u8* pixels = nullptr;

  struct Circle {
    cl_float2 center;
    cl_float radius;
    cl_float3 color;
  };

  struct Rectangle {
    cl_float2 center;
    cl_float2 sizeFromCenter;
    cl_float3 color;
  };

  Circle* hostCircles = nullptr;
  int numCircles = 0;

  Rectangle* hostRectangles = nullptr;
  int numRects = 0;

  cl_device_id device = nullptr;
  size_t maxLocalSize;
  size_t maxDimensions;

  cl_context context;
  cl_command_queue commandQueue;

  cl_mem gpuImage = nullptr;
  cl_mem gpuCircles = nullptr;
  cl_mem gpuRectangles = nullptr;

  cl_kernel kernel;
  cl_program program;

private:
  void createCirclesBuffer(int count);
  void createRectsBuffer(int count);

  void clearHostCicles();
  void clearHostRectangles();

  void clearGpuCicles();
  void clearGpuRectangles();
};

