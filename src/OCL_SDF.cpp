#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#include "CL/cl.h"
#include "utils/utils.hpp"

#include "OCL_SDF.hpp"

#define ATTRIBUTE_COUNT 5

const cl_platform_info attributeTypes[ATTRIBUTE_COUNT] = {
  CL_PLATFORM_NAME,
  CL_PLATFORM_VENDOR,
  CL_PLATFORM_VERSION,
  CL_PLATFORM_PROFILE,
  CL_PLATFORM_EXTENSIONS
};

const char* const attributeNames[ATTRIBUTE_COUNT] = {
  "CL_PLATFORM_NAME",
  "CL_PLATFORM_VENDOR",
  "CL_PLATFORM_VERSION",
  "CL_PLATFORM_PROFILE",
  "CL_PLATFORM_EXTENSIONS"
};

OCL_SDF::OCL_SDF(size_t width, size_t height)
  : width(width), height(height), imageSize(width * height), pixels(new u8[width * height * 4]){

  cl_platform_id platforms[64];
  cl_uint platformCount;

  [[maybe_unused]]
  cl_int platformsResult = clGetPlatformIDs(64, platforms, &platformCount);
  assert(platformsResult == CL_SUCCESS);

  for (cl_uint i = 0; i < platformCount; i++) {
    for (int j = 0; j < ATTRIBUTE_COUNT; j++) {
      // Get platform attribute value size
      size_t infosize = 0;
      [[maybe_unused]]
      cl_int getPlatformInfoResult = clGetPlatformInfo(platforms[i], attributeTypes[j], 0, nullptr, &infosize);
      assert(getPlatformInfoResult == CL_SUCCESS);
      char* info = new char[infosize];

      // Get platform attribute value
      getPlatformInfoResult = clGetPlatformInfo(platforms[i], attributeTypes[j], infosize, info, nullptr);
      assert(getPlatformInfoResult == CL_SUCCESS);
      printf("%d.%d %-11s: %s\n", i+1, j+1, attributeNames[j], info);

      delete[] info;
    }
  }

  printf("\n");

  for (cl_uint i = 0; i < platformCount; i++) {
    cl_device_id devices[64];
    cl_uint deviceCount;
    cl_int deviceResult = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 64, devices, &deviceCount);

    if (deviceResult == CL_SUCCESS) {
      for (cl_uint j = 0; j < deviceCount; j++) {
        char vendorName[256];
        size_t vendorNameLength;
        cl_int deviceInfoResult = clGetDeviceInfo(devices[j], CL_DEVICE_VENDOR, 256, vendorName, &vendorNameLength);
        if (deviceInfoResult == CL_SUCCESS) {
          device = devices[j];
          break;
        }
      }
    }
  }

  assert(device);

  clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(maxDimensions), &maxDimensions, nullptr);
  printf("2.1 CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: %zu\n", maxDimensions);

  clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxLocalSize), &maxLocalSize, nullptr);
  printf("2.2 CL_DEVICE_MAX_WORK_GROUP_SIZE: %zu\n", maxLocalSize);

  size_t* maxDimensionsValues = new size_t[maxDimensions];
  clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, maxDimensions * sizeof(maxDimensionsValues[0]), maxDimensionsValues, nullptr);

  printf("2.3 CL_DEVICE_MAX_WORK_ITEM_SIZES: ");
  for (size_t i = 0; i < maxDimensions; i++) printf("%zu ", maxDimensionsValues[i]);
  printf("\n\n");

  delete[] maxDimensionsValues;

  cl_int contextResult;
  context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &contextResult);
  assert(contextResult == CL_SUCCESS);

  cl_int commandQueueResult;
  commandQueue = clCreateCommandQueueWithProperties(context, device, 0, &commandQueueResult);
  assert(commandQueueResult == CL_SUCCESS);

  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_UNORM_INT8;

  cl_int gpuImageMallocResult;
  #ifdef CL_VERSION_1_2
    cl_image_desc imageDesc;
    memset(&imageDesc, 0, sizeof(imageDesc));
    imageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    imageDesc.image_width = width;
    imageDesc.image_height = height;
    gpuImage = clCreateImage(context, CL_MEM_WRITE_ONLY, &format, &imageDesc, nullptr, &gpuImageMallocResult);
  #else
    gpuImage = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &format, width, height, 0, nullptr, &gpuImageMallocResult);
  #endif
  assert(gpuImageMallocResult == CL_SUCCESS);

  cl_int programResult;
  std::string clFile = readFile("SDF.cl");
  const char* programSource = clFile.c_str();
  size_t programSourceLength = 0;
  program = clCreateProgramWithSource(context, 1, &programSource, &programSourceLength, &programResult);
  assert(programResult == CL_SUCCESS);

  [[maybe_unused]]
  cl_int buildResult = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
  if (buildResult != CL_SUCCESS) {
    size_t logSize;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
    char *log = new char[logSize];
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
    error("Build Log:\n{}\n", log);
  }

  cl_int kernelResult;
  kernel = clCreateKernel(program, "calcSDF", &kernelResult);
  assert(kernelResult == CL_SUCCESS);

  [[maybe_unused]]
  cl_int kernelArgResult = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gpuImage);
  assert(kernelArgResult == CL_SUCCESS);
}

OCL_SDF::~OCL_SDF() {
  if (pixels) delete[] pixels;
  clearHostCicles();
  clearHostRectangles();

  if (gpuImage) clReleaseMemObject(gpuImage);
  clearGpuCicles();
  clearGpuRectangles();

	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(commandQueue);
	clReleaseContext(context);
	clReleaseDevice(device);
}

void OCL_SDF::updateCirclesBuffer(const std::vector<sf::CircleShape>& circles) {
  if (int(circles.size()) != numCircles || gpuCircles == nullptr)
    createCirclesBuffer(circles.size());

  for (int i = 0; i < numCircles; i++) {
    cl_float2 clPos;
    cl_float3 clCol;

    sf::Vector2f pos = circles[i].getPosition();
    sf::Color col = circles[i].getFillColor();

    clPos = {{pos.x, pos.y}};

    clCol.x = col.r / 255.f;
    clCol.y = col.g / 255.f;
    clCol.z = col.b / 255.f;

    hostCircles[i] = {clPos, circles[i].getRadius(), clCol};
  }

  [[maybe_unused]]
  cl_int hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuCircles, CL_TRUE, 0, sizeof(Circle) * numCircles, hostCircles, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);
}

void OCL_SDF::updateRectsBuffer(const std::vector<sf::RectangleShape>& rects) {
  if (int(rects.size()) != numRects || gpuRectangles == nullptr)
    createRectsBuffer(rects.size());

  for (int i = 0; i < numRects; i++) {
    cl_float2 clSizeFromCenter;
    cl_float2 clCenterGlobal;
    cl_float3 clCol;

    sf::Vector2f sizeFromCenter = rects[i].getGeometricCenter();
    sf::Vector2f centerGlobal = rects[i].getPosition() + sizeFromCenter;
    sf::Color col = rects[i].getFillColor();

    clCenterGlobal = {{centerGlobal.x, centerGlobal.y}};
    clSizeFromCenter = {{sizeFromCenter.x, sizeFromCenter.y}};

    clCol.x = col.r / 255.f;
    clCol.y = col.g / 255.f;
    clCol.z = col.b / 255.f;

    hostRectangles[i] = {clCenterGlobal, clSizeFromCenter, clCol};
  }

  [[maybe_unused]]
  cl_int hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuRectangles, CL_TRUE, 0, sizeof(Rectangle) * numRects, hostRectangles, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);
}

void OCL_SDF::run() {
  constexpr size_t origin[3] = {0, 0, 0};
  const size_t region[3] = {width, height, 1};

  const size_t globalWorkSize[2] = {width, height};
  const size_t localWorkSize[2] = {16, 16};

  [[maybe_unused]]
  cl_int errCode;

  errCode = clSetKernelArg(kernel, 1, sizeof(cl_mem), &gpuCircles); assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 2, sizeof(cl_int), &numCircles); assert(errCode == CL_SUCCESS);

  errCode = clSetKernelArg(kernel, 3, sizeof(cl_mem), &gpuRectangles); assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 4, sizeof(cl_int), &numRects);      assert(errCode == CL_SUCCESS);

  errCode = clEnqueueNDRangeKernel(commandQueue, kernel, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, nullptr); assert(errCode == CL_SUCCESS);
  errCode = clEnqueueReadImage(commandQueue, gpuImage, CL_TRUE, origin, region, 0, 0, pixels, 0, nullptr, nullptr);       assert(errCode == CL_SUCCESS);

  errCode = clFinish(commandQueue); assert(errCode == CL_SUCCESS);
}

const u8* OCL_SDF::getPixels() const {
  return pixels;
}

void OCL_SDF::createCirclesBuffer(int count) {
  clearHostCicles();
  clearGpuCicles();

  numCircles = count;
  hostCircles = new Circle[numCircles];

  cl_int gpuMallocResult;
  gpuCircles = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Circle) * numCircles, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);
}

void OCL_SDF::createRectsBuffer(int count) {
  clearHostRectangles();
  clearGpuRectangles();

  numRects = count;
  hostRectangles = new Rectangle[numRects];

  cl_int gpuMallocResult;
  gpuRectangles = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Rectangle) * numRects, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);
}

void OCL_SDF::clearHostCicles()     { if (hostCircles)    delete[] hostCircles;    }
void OCL_SDF::clearHostRectangles() { if (hostRectangles) delete[] hostRectangles; }

void OCL_SDF::clearGpuCicles()      { if (gpuCircles)    clReleaseMemObject(gpuCircles);    }
void OCL_SDF::clearGpuRectangles()  { if (gpuRectangles) clReleaseMemObject(gpuRectangles); }

