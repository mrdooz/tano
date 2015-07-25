#include "common.hlsl"

cbuffer F : register(b0)
{
  float2 tonemap; // x = exposure/lumAvg, y = min_white
};

float4 PsComposite(VSQuadOut p) : SV_Target
{
  // Texture0 : color
  // Texture1 : depth buffer
  float2 uv = p.uv.xy;

  float4 col = Texture0.Sample(PointSampler, uv); 
  float zBuf = Texture1.Load(int3(p.pos.x, p.pos.y, 0)).r;

  float exposure = tonemap.x;
  float minWhite = tonemap.y;

  col = ToneMapReinhard(col, exposure, minWhite);
  
   // gamma correction
  col = pow(abs(col), 1.0/2.2);

  float2 xx = -1 + 2 * uv;
  float r = 0.7 + 0.9 - smoothstep(0, 1, sqrt(xx.x*xx.x + xx.y*xx.y));
  return r * col;
}