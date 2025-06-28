#include <vector>

#include "CL/cl.h"
#include "utils/types.hpp"

class OCL_RayMarching {
public:
  OCL_RayMarching(size_t width, size_t height);
  ~OCL_RayMarching();

  void updateCirclesBuffer(const std::vector<sf::CircleShape>& circles);
  void updateRectsBuffer(const std::vector<sf::RectangleShape>& rects);

  void run();

  [[nodiscard]]
  const u8* getPixels() const;

private:
  const size_t width, height;
  const size_t imageSize;
  u8* pixels = nullptr;

  cl_float2* hostCirclesCenters = nullptr;
  cl_float* hostCirclesRadii = nullptr;
  int numCircles = 0;

  cl_float2* hostRectsCenters = nullptr;
  cl_float2* hostRectsSizesFromCenters = nullptr;
  int numRects = 0;

  cl_device_id device = nullptr;
  size_t maxLocalSize;
  size_t maxDimensions;

  cl_context context;
  cl_command_queue commandQueue;

  cl_mem gpuImage = nullptr;

  cl_mem gpuCirclesCenters = nullptr;
  cl_mem gpuCirclesRadii = nullptr;

  cl_mem gpuRectsCenters = nullptr;
  cl_mem gpuRectsSizesFromCenters = nullptr;

  cl_kernel kernel;
  cl_program program;

private:
  void createCirclesBuffer(int count);
  void createRectsBuffer(int count);

  void clearHostCircles();
  void clearHostRects();
  void clearGpuCircles();
  void clearGpuRects();
};

