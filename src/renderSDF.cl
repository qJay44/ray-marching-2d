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
  float maxDist = length((float2)(width, height));

  if (x > width - 1 || y > height - 1) return;

  float minDstCircle = FLT_MAX;
  float minDstRect = FLT_MAX;
  float3 minDstCircleColor = 0.f;
  float3 minDstRectColor = 0.f;

  for (int i = 0; i < numCircles; i++) {
    Circle circle = circles[i];
    float sdf = signedDstToCircle(point, circle);
    if (sdf < minDstCircle) {
      float t = smoothstep(0.f, 1.f, fabs(sdf) / maxDist);
      minDstCircle = sdf;
      minDstCircleColor = mix(circle.color, minDstCircleColor, pow(t, k));
    }
  }

  for (int i = 0; i < numRectangles; i++) {
    Rectangle rect = rectangles[i];
    float sdf = signedDstToRectangle(point, rect);
    if (sdf < minDstRect) {
      float t = smoothstep(0.f, 1.f, fabs(sdf) / maxDist);
      minDstRect = sdf;
      minDstRectColor = mix(rect.color, minDstRectColor, pow(t, k));
    }
  }

  float t = fabs(min(minDstCircle, minDstRect)) / maxDist;
  t = smoothstep(0.f, 1.f, t);
  float3 col = mix(minDstCircleColor, minDstRectColor, pow(t, k));

  write_imagef(img, (int2)(x, y), (float4)(col, 1.f));
}

