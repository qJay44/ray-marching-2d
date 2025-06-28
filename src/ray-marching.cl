constant float2 f2zero = (float2)(0.f, 0.f);

float signedDstToCircle(const float2 p, const float2 center, float r) {
  return length(center - p) - r;
}

float signedDstToRectangle(const float2 p, const float2 center, const float2 size) {
  float2 v = p - center;
  float2 offset = fabs(v) - size;
  float unsignedDst = length(fmax(offset, f2zero));

  float2 dstInsideBoxV = fmin(offset, f2zero);
  float dstInsideBox = fmax(dstInsideBoxV.x, dstInsideBoxV.y);

  return unsignedDst + dstInsideBox;
}

__kernel void renderSDF(
  __write_only image2d_t img,
  __global const float2* circlesCenters, __global const float* circlesRadii, const int nCircles,
  __global const float2* rectsCenters, __global const float2* rectsSizesFromCenters, const int nRects
) {
  int x = get_global_id(0);
  int y = get_global_id(1);

  int width = get_image_width(img);
  int height = get_image_height(img);

  if (x > width - 1 || y > height - 1) return;

  float minDist = FLT_MAX;
  float2 p = (float2)(x, y);

  for (int j = 0; j < nCircles; j++)
    minDist = fmin(minDist, signedDstToCircle(p, circlesCenters[j], circlesRadii[j]));

  for (int j = 0; j < nRects; j++)
    minDist = fmin(minDist, signedDstToRectangle(p, rectsCenters[j], rectsSizesFromCenters[j]));

  float colorVal = clamp(0.5f + minDist * 0.1f, 0.f, 1.f);
  float4 color = (float4)(colorVal, colorVal, colorVal, 1.f);

  write_imagef(img, (int2)(x, y), color);
}

