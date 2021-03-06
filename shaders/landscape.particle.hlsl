#include "landscape.hlsl"

cbuffer P : register(b0)
{
  float4 nearFar : NEAR_FAR;
  float3 sunDir : SUN_DIR;
};

cbuffer G : register(b0)
{
  matrix world;
  matrix viewProj;
  float3 cameraPos;
  float4 time;
};

struct VsParticleIn
{
  float3 pos : Position;
};

struct GsParticleOut
{
  float4 pos : SV_Position;
  float2 uv : TexCoord0;
  float z : TexCoord1;
  float brightness : TexCoord2;
  float3 rayDir : TexCoord3;
};

// 1--2
// |  |
// 0--3

static float2 uvsVtx[4] = {
  float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
};

// entry-point: vs
VsParticleIn VsParticle(VsParticleIn v)
{
  return v;
}

[maxvertexcount(4)]
// entry-point: gs
void GsParticle(point VsParticleIn input[1], inout TriangleStream<GsParticleOut> outStream)
{
  // Note, the DirectX strip order differs from my usual order. It might be
  // a good idea to change my stuff..
  // 1--3
  // |  |
  // 0--2

  matrix worldViewProj = mul(world, viewProj);

  float3 pos = input[0].pos.xyz;
  float3 dir = normalize(cameraPos - pos);
  float3 right = cross(dir, float3(0,1,0));
  float3 up = cross(right, dir);

  GsParticleOut p;
  p.brightness = saturate(1 + 1.5 * sin(time.x + pos.x));
  p.rayDir = dir;
  float s = 0.75;
  float3 p0 = float3(pos - s * right - s * up);
  float3 p1 = float3(pos - s * right + s * up);
  float3 p2 = float3(pos + s * right - s * up);
  float3 p3 = float3(pos + s * right + s * up);
  p.pos = mul(float4(p0, 1), worldViewProj);
  p.uv = uvsVtx[0];
  p.z = p.pos.z;
  outStream.Append(p);

  p.pos = mul(float4(p1, 1), worldViewProj);
  p.uv = uvsVtx[1];
  p.z = p.pos.z;
  outStream.Append(p);

  p.pos = mul(float4(p2, 1), worldViewProj);
  p.uv = uvsVtx[2];
  p.z = p.pos.z;
  outStream.Append(p);

  p.pos = mul(float4(p3, 1), worldViewProj);
  p.uv = uvsVtx[3];
  p.z = p.pos.z;
  outStream.Append(p);
}

static float SoftParticleContrast = 2.0;
static float intensity = 1.0;
static float zEpsilon = 0.0;

// entry-point: ps
PsColBrightnessOut PsParticle(GsParticleOut p)
{
  PsColBrightnessOut res;

  // Texture0 = particle texture
  // Texture1 = zbuffer
  float2 uv = p.uv.xy;
  float4 col = Texture0.Sample(LinearSampler, uv);
  float zBuf = Texture1.Load(int3(p.pos.x, p.pos.y, 0)).r;

  // f*(z-n) / (f-n)*z = zbuf => z = f*n / (f-zbuf(f-n))
  float farClip = nearFar.y;
  float f_mul_n = nearFar.z;
  float f_sub_n = nearFar.w;
  float z = f_mul_n / ( farClip - zBuf * f_sub_n);
  float zdiff = (z - p.z);
  float c = smoothstep(0, 1, zdiff / SoftParticleContrast);
  if( c * zdiff <= zEpsilon )
      discard;

  col = intensity * c * col;

  // Apply fog
  float fogAmount = max(0, 1 - exp(-(p.z) * FOG_SCALE));
  float3 fogColor = FogColor(p.rayDir, sunDir);
  col.xyz = lerp(col.xyz, col.xyz * fogColor, fogAmount);

  res.col = p.brightness * col;

  res.emissive.rgb = 0;
  res.emissive.a = 0; //Luminance(intensity * c * col.rgb);
  return res;
}
