typedef struct {
  float2 center;
  float radius;
  float3 color;
} Circle;

typedef struct {
  float2 center;
  float2 sizeFromCenter;
  float3 color;
} Rectangle;

float signedDstToCircle(const float2 point, const Circle circle) {
  return length(circle.center - point) - circle.radius;
}

float signedDstToRectangle(const float2 point, const Rectangle rect) {
  float2 v = point - rect.center;
  float2 offset = fabs(v) - rect.sizeFromCenter;
  float unsignedDst = length(fmax(offset, 0.f));

  float2 dstInsideBoxV = fmin(offset, 0.f);
  float dstInsideBox = fmax(dstInsideBoxV.x, dstInsideBoxV.y);

  return unsignedDst + dstInsideBox;
}

// cubic polynomial
float smin(float a, float b, float k) {
  k *= 6.f;
  float h = max(k - fabs(a - b), 0.f) / k;
  return min(a, b) - h * h * h * k * (1.f / 6.f);
}

__kernel void renderSDF(
  __write_only image2d_t img,
  __global const Circle* circles, const int numCircles,
  __global const Rectangle* rectangles, const int numRectangles,
  const float k
) {
  int x = get_global_id(0);
  int y = get_global_id(1);
  float2 point = (float2)(x, y);

  int width = get_image_width(img);
  int height = get_image_height(img);

  if (x > width - 1 || y > height - 1) return;

  float minDstCircle = FLT_MAX;
  float minDstRect = FLT_MAX;
  float3 minDstCircleColor = 0.f;
  float3 minDstRectColor = 0.f;

  const float edgeThreshold = 0.01f;
  const float smoothing = 0.005f;

  for (int i = 0; i < numCircles; i++) {
    Circle circle = circles[i];
    float sdf = signedDstToCircle(point, circle);
    minDstCircle = smin(minDstCircle, sdf, k);
    float alpha = 1.f - smoothstep(edgeThreshold - smoothing, edgeThreshold + smoothing, sdf);
    minDstCircleColor = mix(minDstCircleColor, circle.color, alpha);
  }

  for (int i = 0; i < numRectangles; i++) {
    Rectangle rect = rectangles[i];
    float sdf = signedDstToRectangle(point, rect);
    minDstRect = smin(minDstRect, sdf, k);
    float alpha = 1.f - smoothstep(edgeThreshold - smoothing, edgeThreshold + smoothing, sdf);
    minDstRectColor = mix(minDstRectColor, rect.color, alpha);
  }

  float alphaCircle = 1.f - smoothstep(edgeThreshold - smoothing, edgeThreshold + smoothing, minDstCircle);
  float alphaRect = 1.f - smoothstep(edgeThreshold - smoothing, edgeThreshold + smoothing, minDstRect);
  float totalAlpha = alphaCircle + alphaRect;
  totalAlpha = clamp(totalAlpha, 0.f, 1.f);

  float ratio = alphaRect / (totalAlpha + 1e-5f);
  float finalAlpha = max(alphaCircle, alphaRect);

  float3 col = mix(minDstCircleColor, minDstRectColor, ratio);
  float4 color = (float4)(col, finalAlpha);

  write_imagef(img, (int2)(x, y), color);
}

