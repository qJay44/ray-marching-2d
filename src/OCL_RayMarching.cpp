#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#include "utils/utils.hpp"

#include "OCL_RayMarching.hpp"

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

OCL_RayMarching::OCL_RayMarching(size_t width, size_t height)
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
  std::string clFile = readFile("ray-marching.cl");
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
  kernel = clCreateKernel(program, "renderSDF", &kernelResult);
  assert(kernelResult == CL_SUCCESS);

  [[maybe_unused]]
  cl_int kernelArgResult1 = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gpuImage);
  assert(kernelArgResult1 == CL_SUCCESS);
}

OCL_RayMarching::~OCL_RayMarching() {
  if (pixels) delete[] pixels;
  if (gpuImage) clReleaseMemObject(gpuImage);

  clearHostCircles();
  clearHostRects();

  clearGpuCircles();
  clearGpuRects();

	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(commandQueue);
	clReleaseContext(context);
	clReleaseDevice(device);
}

void OCL_RayMarching::updateCirclesBuffer(const std::vector<sf::CircleShape>& circles) {
  // gpuCirclesCenters always being created along with gpuCirclesRadii
  if (int(circles.size()) != numCircles || gpuCirclesCenters == nullptr)
    createCirclesBuffer(circles.size());

  for (int i = 0; i < numCircles; i++) {
    sf::Vector2f pos = circles[i].getPosition();
    sf::Color col = circles[i].getFillColor();
    cl_float2 clPos;
    cl_float3 clCol;

    clPos.x = pos.x;
    clPos.y = pos.y;

    clCol.x = col.r / 255.f;
    clCol.y = col.g / 255.f;
    clCol.z = col.b / 255.f;

    hostCirclesCenters[i] = clPos;
    hostCirclesRadii[i] = circles[i].getRadius();
    hostCirclesColors[i] = clCol;
  }

  [[maybe_unused]]
  cl_int hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuCirclesCenters, CL_TRUE, 0, sizeof(cl_float2) * numCircles, hostCirclesCenters, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);

  hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuCirclesRadii, CL_TRUE, 0, sizeof(cl_float) * numCircles, hostCirclesRadii, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);

  hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuCirclesColors, CL_TRUE, 0, sizeof(cl_float3) * numCircles, hostCirclesColors, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);
}

void OCL_RayMarching::updateRectsBuffer(const std::vector<sf::RectangleShape>& rects) {
  // gpuRectsCenters always being created along with gpuRectsSizesFromCenters
  if (int(rects.size()) != numRects || gpuRectsCenters == nullptr)
    createRectsBuffer(rects.size());

  for (int i = 0; i < numRects; i++) {
    cl_float2 clSizeFromCenter;
    cl_float2 clCenterGlobal;
    cl_float3 clCol;
    sf::Vector2f sizeFromCenter = rects[i].getGeometricCenter();
    sf::Vector2f centerGlobal = rects[i].getPosition() + sizeFromCenter;
    sf::Color col = rects[i].getFillColor();

    clCenterGlobal.x = centerGlobal.x;
    clCenterGlobal.y = centerGlobal.y;

    clSizeFromCenter.x = sizeFromCenter.x;
    clSizeFromCenter.y = sizeFromCenter.y;

    clCol.x = col.r / 255.f;
    clCol.y = col.g / 255.f;
    clCol.z = col.b / 255.f;

    hostRectsCenters[i] = clCenterGlobal;
    hostRectsSizesFromCenters[i] = clSizeFromCenter;
    hostRectsColors[i] = clCol;
  }

  [[maybe_unused]]
  cl_int hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuRectsCenters, CL_TRUE, 0, sizeof(cl_float2) * numRects, hostRectsCenters, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);

  hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuRectsSizesFromCenters, CL_TRUE, 0, sizeof(cl_float2) * numRects, hostRectsSizesFromCenters, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);

  hostCopyResult = clEnqueueWriteBuffer(commandQueue, gpuRectsColors, CL_TRUE, 0, sizeof(cl_float3) * numRects, hostRectsColors, 0, nullptr, nullptr);
  assert(hostCopyResult == CL_SUCCESS);
}

void OCL_RayMarching::run(float k) {
  constexpr size_t origin[3] = {0, 0, 0};
  const size_t region[3] = {width, height, 1};

  const size_t globalWorkSize[2] = {width, height};
  const size_t localWorkSize[2] = {16, 16};

  [[maybe_unused]]
  cl_int errCode;

  errCode = clSetKernelArg(kernel, 1, sizeof(cl_mem), &gpuCirclesCenters); assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 2, sizeof(cl_mem), &gpuCirclesRadii);   assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 3, sizeof(cl_mem), &gpuCirclesColors);  assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 4, sizeof(cl_int), &numCircles);        assert(errCode == CL_SUCCESS);

  errCode = clSetKernelArg(kernel, 5, sizeof(cl_mem), &gpuRectsCenters);          assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 6, sizeof(cl_mem), &gpuRectsSizesFromCenters); assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 7, sizeof(cl_mem), &gpuRectsColors);           assert(errCode == CL_SUCCESS);
  errCode = clSetKernelArg(kernel, 8, sizeof(cl_int), &numRects);                 assert(errCode == CL_SUCCESS);

  errCode = clSetKernelArg(kernel, 9, sizeof(cl_float), &k); assert(errCode == CL_SUCCESS);

  errCode = clEnqueueNDRangeKernel(commandQueue, kernel, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, nullptr); assert(errCode == CL_SUCCESS);
  errCode = clEnqueueReadImage(commandQueue, gpuImage, CL_TRUE, origin, region, 0, 0, pixels, 0, nullptr, nullptr);       assert(errCode == CL_SUCCESS);

  errCode = clFinish(commandQueue); assert(errCode == CL_SUCCESS);
}

const u8* OCL_RayMarching::getPixels() const {
  return pixels;
}

void OCL_RayMarching::createCirclesBuffer(int count) {
  clearHostCircles();
  clearGpuCircles();

  numCircles = count;
  hostCirclesCenters = new cl_float2[numCircles];
  hostCirclesRadii = new cl_float[numCircles];
  hostCirclesColors = new cl_float3[numCircles];

  cl_int gpuMallocResult;
  gpuCirclesCenters = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float2) * numCircles, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);

  gpuCirclesRadii = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float) * numCircles, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);

  gpuCirclesColors = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float3) * numCircles, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);
}

void OCL_RayMarching::createRectsBuffer(int count) {
  clearGpuRects();
  clearGpuRects();

  numRects = count;
  hostRectsCenters = new cl_float2[numRects];
  hostRectsSizesFromCenters = new cl_float2[numRects];
  hostRectsColors = new cl_float3[numRects];

  cl_int gpuMallocResult;
  gpuRectsCenters = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float2) * numRects, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);

  gpuRectsSizesFromCenters = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float2) * numRects, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);

  gpuRectsColors = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float3) * numRects, nullptr, &gpuMallocResult);
  assert(gpuMallocResult == CL_SUCCESS);
}

void OCL_RayMarching::clearHostCircles() {
  if (hostCirclesCenters) delete[] hostCirclesCenters;
  if (hostCirclesRadii)   delete[] hostCirclesRadii;
  if (hostCirclesColors)  delete[] hostCirclesColors;
}

void OCL_RayMarching::clearHostRects() {
  if (hostRectsCenters)          delete[] hostRectsCenters;
  if (hostRectsSizesFromCenters) delete[] hostRectsSizesFromCenters;
  if (hostRectsColors)           delete[] hostRectsColors;
}

void OCL_RayMarching::clearGpuCircles() {
  if (gpuCirclesCenters) clReleaseMemObject(gpuCirclesCenters);
  if (gpuCirclesRadii)   clReleaseMemObject(gpuCirclesRadii);
  if (gpuCirclesColors)  clReleaseMemObject(gpuCirclesColors);
}

void OCL_RayMarching::clearGpuRects() {
  if (gpuRectsCenters)          clReleaseMemObject(gpuRectsCenters);
  if (gpuRectsSizesFromCenters) clReleaseMemObject(gpuRectsSizesFromCenters);
  if (gpuRectsColors)           clReleaseMemObject(gpuRectsColors);
}

