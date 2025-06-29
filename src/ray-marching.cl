// cubic polynomial
float smin(float a, float b, float k) {
  k *= 6.f;
  float h = max(k - fabs(a - b), 0.f) / k;
  return min(a, b) - h * h * h * k * (1.f / 6.f);
}

float signedDstToCircle(const float2 p, const float2 center, float r) {
  return length(center - p) - r;
}

float signedDstToRectangle(const float2 p, const float2 center, const float2 size) {
  float2 v = p - center;
  float2 offset = fabs(v) - size;
  float unsignedDst = length(fmax(offset, 0.f));

  float2 dstInsideBoxV = fmin(offset, 0.f);
  float dstInsideBox = fmax(dstInsideBoxV.x, dstInsideBoxV.y);

  return unsignedDst + dstInsideBox;
}

__kernel void renderSDF(
  __write_only image2d_t img,
  __global const float2* circlesCenters, __global const float* circlesRadii, __global const float3* circlesColors, const int nCircles,
  __global const float2* rectsCenters, __global const float2* rectsSizesFromCenters, __global const float3* rectsColors, const int nRects,
  const float k
) {
  int x = get_global_id(0);
  int y = get_global_id(1);
  float2 p = (float2)(x, y);

  int width = get_image_width(img);
  int height = get_image_height(img);

  if (x > width - 1 || y > height - 1) return;

  float minDstCircle = FLT_MAX;
  float minDstRect = FLT_MAX;
  float3 minDstCircleColor = 0.f;
  float3 minDstRectColor = 0.f;

  for (int i = 0; i < nCircles; i++) {
    float sdf = signedDstToCircle(p, circlesCenters[i], circlesRadii[i]);
    if (sdf < minDstCircle) {
      minDstCircle = sdf;
      minDstCircleColor = circlesColors[i];
    }
  }

  for (int i = 0; i < nRects; i++) {
    float sdf = signedDstToRectangle(p, rectsCenters[i], rectsSizesFromCenters[i]);
    if (sdf < minDstRect) {
      minDstRect = sdf;
      minDstRectColor = rectsColors[i];
    }
  }

  const float edgeThreshold = 0.01f;
  const float smoothing = 0.005f;

  float alphaCircle = 1.f - smoothstep(edgeThreshold - smoothing, edgeThreshold + smoothing, minDstCircle);
  float alphaRect = 1.f - smoothstep(edgeThreshold - smoothing, edgeThreshold + smoothing, minDstRect);
  float combinedAlpha = max(alphaCircle, alphaRect);

  float3 col = mix(minDstCircleColor, minDstRectColor, alphaRect / (alphaCircle + alphaRect + 1e-5f));
  float4 color = (float4)(col, combinedAlpha);

  write_imagef(img, (int2)(x, y), color);
}

