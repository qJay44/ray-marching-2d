#define PI 3.14159265359f
#define TAU (2.f * PI)

uniform sampler2D u_baseTexture;
uniform sampler2D u_sdfTexture;
uniform sampler2D u_blueNoiseTexture;
uniform vec2 u_resolution;
uniform int u_stepsPerRay;
uniform int u_raysPerPixel;
uniform float u_epsilon;

vec2 uvStep = 1.f / u_resolution;

vec3 rayMarch(vec2 pix, vec2 dir) {
  float dist = 0.f;
  for (int i = 0; i < u_stepsPerRay; i++) {
    dist = texture2D(u_sdfTexture, vec2(pix.x, 1.f - pix.y)).r;
    pix += dir * dist;

    bool offscreen = pix.x > 1.f || pix.x < 0.f || pix.y > 1.f || pix.y < 0.f;

    if (dist < u_epsilon || offscreen) {
      return max(
        texture2D(u_baseTexture, pix).rgb,
        texture2D(u_baseTexture, pix - (dir * uvStep)).rgb
      );
    }
  }

  return vec3(0.f);
}

void main() {
  vec2 uv = vec2(gl_FragCoord.xy) / u_resolution;

  float dist = texture2D(u_sdfTexture, vec2(uv.x, 1.f - uv.y)).r;
  vec3 light = texture2D(u_baseTexture, uv).rgb;

  if (dist >= u_epsilon) {
    float brightness = max(light.r, max(light.g, light.b));
    float noise = TAU * texture2D(u_blueNoiseTexture, uv).r;
    float step = TAU / u_raysPerPixel;

    for (float angle = 0.f; angle < TAU; angle += step) {
      vec2 rayDir = vec2(cos(angle + noise), sin(angle + noise));
      vec3 hitColor = rayMarch(uv, rayDir);
      light += hitColor;
      brightness += max(hitColor.r, max(hitColor.g, hitColor.b));
    }

    light = light / brightness * brightness / u_raysPerPixel;
  }

  gl_FragColor = vec4(light, 1.f);
}

