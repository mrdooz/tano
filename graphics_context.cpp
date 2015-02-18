#include "graphics.hpp"
#include "graphics_context.hpp"
#include "gpu_objects.hpp"

static const int MAX_SAMPLERS = 8;
static const int MAX_TEXTURES = 8;

using namespace tano;

//------------------------------------------------------------------------------
GraphicsContext::GraphicsContext(ID3D11DeviceContext* ctx)
  : _ctx(ctx)
  , _default_stencil_ref(0)
  , _default_sample_mask(0xffffffff)
{
    _default_blend_factors[0] = _default_blend_factors[1] = _default_blend_factors[2] = _default_blend_factors[3] = 1;
}

//------------------------------------------------------------------------------
void GraphicsContext::GenerateMips(ObjectHandle h)
{
  auto r = GRAPHICS._renderTargets.Get(h)->srv.resource;
  _ctx->GenerateMips(r);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetRenderTarget(ObjectHandle render_target, const Color* clearColor)
{
  SetRenderTargets(&render_target, clearColor, 1);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetRenderTargets(
    ObjectHandle* renderTargets,
    const Color* clearTargets,
    int numRenderTargets)
{
  if (!numRenderTargets)
    return;

  ID3D11RenderTargetView *rts[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
  ID3D11DepthStencilView *dsv = nullptr;
  D3D11_TEXTURE2D_DESC texture_desc;

  // Collect the valid render targets, set the first available depth buffer
  // and clear targets if specified
  for (int i = 0; i < numRenderTargets; ++i)
  {
    ObjectHandle h = renderTargets[i];
    assert(h.IsValid());
    RenderTargetResource* rt = GRAPHICS._renderTargets.Get(h);
    texture_desc = rt->texture.desc;
    if (!dsv && rt->dsv.resource)
    {
      dsv = rt->dsv.resource;
    }
    rts[i] = rt->rtv.resource;
    // clear render target (and depth stenci)
    if (clearTargets[i])
    {
      _ctx->ClearRenderTargetView(rts[i], &clearTargets[i].x);
      if (rt->dsv.resource)
      {
        _ctx->ClearDepthStencilView(rt->dsv.resource, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
      }
    }
  }
  CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)texture_desc.Width, (float)texture_desc.Height);
  _ctx->RSSetViewports(1, &viewport);
  _ctx->OMSetRenderTargets(numRenderTargets, rts, dsv);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetSwapChain(ObjectHandle h, const Color& clearColor)
{
  SetSwapChain(h, (const float*)clearColor);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetSwapChain(ObjectHandle h, const float* clearColor)
{
  auto swapChain  = GRAPHICS._swapChains.Get(h);
  auto rt         = GRAPHICS._renderTargets.Get(swapChain->_renderTarget);
  _ctx->OMSetRenderTargets(1, &rt->rtv.resource.p, rt->dsv.resource);

  if (clearColor)
  {
    _ctx->ClearRenderTargetView(rt->rtv.resource, clearColor);
    _ctx->ClearDepthStencilView(rt->dsv.resource, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
  }
  _ctx->RSSetViewports(1, &swapChain->_viewport);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetVertexShader(ObjectHandle vs)
{
  assert(vs.type() == ObjectHandle::kVertexShader || !vs.IsValid());
  _ctx->VSSetShader(vs.IsValid() ? GRAPHICS._vertexShaders.Get(vs) : nullptr, nullptr, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetComputeShader(ObjectHandle cs)
{
  assert(cs.type() == ObjectHandle::kComputeShader || !cs.IsValid());
  _ctx->CSSetShader(cs.IsValid() ? GRAPHICS._computeShaders.Get(cs) : nullptr, nullptr, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetGeometryShader(ObjectHandle gs)
{
  assert(gs.type() == ObjectHandle::kGeometryShader || !gs.IsValid());
  _ctx->GSSetShader(gs.IsValid() ? GRAPHICS._geometryShaders.Get(gs) : nullptr, nullptr, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetPixelShader(ObjectHandle ps)
{
  assert(ps.type() == ObjectHandle::kPixelShader || !ps.IsValid());
  _ctx->PSSetShader(ps.IsValid() ? GRAPHICS._pixelShaders.Get(ps) : nullptr, nullptr, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetLayout(ObjectHandle layout)
{
  _ctx->IASetInputLayout(layout.IsValid() ? GRAPHICS._inputLayouts.Get(layout) : nullptr);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetIndexBuffer(ObjectHandle ib)
{
  if (ib.IsValid())
    SetIndexBuffer(GRAPHICS._indexBuffers.Get(ib), (DXGI_FORMAT)ib.data());
  else
    SetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetIndexBuffer(ID3D11Buffer* buf, DXGI_FORMAT format)
{
  _ctx->IASetIndexBuffer(buf, format, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetVertexBuffer(ID3D11Buffer* buf, u32 stride)
{
  UINT ofs[] = { 0 };
  ID3D11Buffer* bufs[] = { buf };
  u32 strides[] = { stride };
  _ctx->IASetVertexBuffers(0, 1, bufs, strides, ofs);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetVertexBuffer(ObjectHandle vb) 
{
  if (vb.IsValid())
    SetVertexBuffer(GRAPHICS._vertexBuffers.Get(vb), vb.data());
  else
    SetVertexBuffer(nullptr, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY top)
{
  _ctx->IASetPrimitiveTopology(top);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetRasterizerState(ObjectHandle rs)
{
  _ctx->RSSetState(GRAPHICS._rasterizerStates.Get(rs));
}

//------------------------------------------------------------------------------
void GraphicsContext::SetDepthStencilState(ObjectHandle dss, UINT stencil_ref)
{
  _ctx->OMSetDepthStencilState(GRAPHICS._depthStencilStates.Get(dss), stencil_ref);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetBlendState(ObjectHandle bs, const float* blendFactors, UINT sampleMask)
{
  _ctx->OMSetBlendState(GRAPHICS._blendStates.Get(bs), blendFactors, sampleMask);
}

//------------------------------------------------------------------------------
void GraphicsContext::UnsetShaderResources(u32 first, u32 count, ShaderType shaderType)
{
  ID3D11ShaderResourceView* srViews[16] = { nullptr };

  if (shaderType == ShaderType::VertexShader)
    _ctx->VSSetShaderResources(first, count, srViews);
  else if (shaderType == ShaderType::PixelShader)
    _ctx->PSSetShaderResources(first, count, srViews);
  else if (shaderType == ShaderType::ComputeShader)
    _ctx->CSSetShaderResources(first, count, srViews);
  else if (shaderType == ShaderType::GeometryShader)
    _ctx->GSSetShaderResources(first, count, srViews);
}

//------------------------------------------------------------------------------
void GraphicsContext::UnsetUnorderedAccessViews(int first, int count)
{
  UINT initialCount = -1;
  static ID3D11UnorderedAccessView *nullViews[MAX_TEXTURES] = { nullptr };
  _ctx->CSSetUnorderedAccessViews(first, count, nullViews, &initialCount);
}

//------------------------------------------------------------------------------
void GraphicsContext::UnsetRenderTargets(int first, int count)
{
  static ID3D11RenderTargetView *nullViews[8] = { nullptr };
  _ctx->OMSetRenderTargets(count, nullViews, nullptr);
}

//------------------------------------------------------------------------------
bool GraphicsContext::Map(
  ObjectHandle h,
  UINT sub,
  D3D11_MAP type,
  UINT flags,
  D3D11_MAPPED_SUBRESOURCE *res)
{
  switch (h.type())
  {
  case ObjectHandle::kTexture:
    return SUCCEEDED(_ctx->Map(GRAPHICS._textures.Get(h)->texture.resource, sub, type, flags, res));

  case ObjectHandle::kVertexBuffer:
    return SUCCEEDED(_ctx->Map(GRAPHICS._vertexBuffers.Get(h), sub, type, flags, res));

  case ObjectHandle::kIndexBuffer:
    return SUCCEEDED(_ctx->Map(GRAPHICS._indexBuffers.Get(h), sub, type, flags, res));

  default:
    //LOG_ERROR_LN("Invalid resource type passed to %s", __FUNCTION__);
    return false;
  }
}

//------------------------------------------------------------------------------
void GraphicsContext::Unmap(ObjectHandle h, UINT sub)
{
  switch (h.type())
  {
  case ObjectHandle::kTexture:
    _ctx->Unmap(GRAPHICS._textures.Get(h)->texture.resource, sub);
    break;

  case ObjectHandle::kVertexBuffer:
    _ctx->Unmap(GRAPHICS._vertexBuffers.Get(h), sub);
    break;

  case ObjectHandle::kIndexBuffer:
    _ctx->Unmap(GRAPHICS._indexBuffers.Get(h), sub);
    break;

  default:
    break;
    //LOG_WARNING_LN("Invalid resource type passed to %s", __FUNCTION__);
  }
}

//------------------------------------------------------------------------------
void GraphicsContext::CopyToBuffer(ObjectHandle h, const void* data, u32 len)
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (Map(h, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, data, len);
    Unmap(h, 0);
  }
}

//------------------------------------------------------------------------------
void GraphicsContext::CopyToBuffer(
    ObjectHandle h,
    UINT sub,
    D3D11_MAP type,
    UINT flags,
    const void* data,
    u32 len)
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (Map(h, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, data, len);
    Unmap(h, 0);
  }
}

//------------------------------------------------------------------------------
void GraphicsContext::DrawIndexed(int count, int start_index, int base_vertex)
{
  _ctx->DrawIndexed(count, start_index, base_vertex);
}

//------------------------------------------------------------------------------
void GraphicsContext::Draw(int vertexCount, int startVertexLocation)
{
  _ctx->Draw(vertexCount, startVertexLocation);
} 

//------------------------------------------------------------------------------
void GraphicsContext::Dispatch(int threadGroupCountX, int threadGroupCountY, int threadGroupCountZ)
{
  _ctx->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

//------------------------------------------------------------------------------
void GraphicsContext::Flush()
{
  _ctx->Flush();
}

//------------------------------------------------------------------------------
void GraphicsContext::SetShaderResources(
    const vector<ObjectHandle>& handles,
    ShaderType shaderType)
{
  ID3D11ShaderResourceView** v = (ID3D11ShaderResourceView**)_alloca(sizeof(ID3D11ShaderResourceView*) * handles.size());
  ID3D11ShaderResourceView** t = v;
  for (auto h : handles)
  {
    *t++ = GRAPHICS.GetShaderResourceView(h);
  }

  u32 count = (u32)handles.size();
  if (shaderType == ShaderType::VertexShader)
    _ctx->VSSetShaderResources(0, count, v);
  else if (shaderType == ShaderType::PixelShader)
    _ctx->PSSetShaderResources(0, count, v);
  else if (shaderType == ShaderType::ComputeShader)
    _ctx->CSSetShaderResources(0, count, v);
  else if (shaderType == ShaderType::GeometryShader)
    _ctx->GSSetShaderResources(0, count, v);
  else
    assert(false);

}

//------------------------------------------------------------------------------
void GraphicsContext::SetShaderResource(ObjectHandle h, ShaderType shaderType, int slot)
{
  ID3D11ShaderResourceView* view = view = GRAPHICS.GetShaderResourceView(h);
  view = GRAPHICS.GetShaderResourceView(h);
  if (!view)
  {
    LOG_ERROR("Unable to get shader resource view");
    return;
  }

  if (shaderType == ShaderType::VertexShader)
    _ctx->VSSetShaderResources(slot, 1, &view);
  else if (shaderType == ShaderType::PixelShader)
    _ctx->PSSetShaderResources(slot, 1, &view);
  else if (shaderType == ShaderType::ComputeShader)
    _ctx->CSSetShaderResources(slot, 1, &view);
  else if (shaderType == ShaderType::GeometryShader)
    _ctx->GSSetShaderResources(slot, 1, &view);
  else
    assert(false);
    //LOG_ERROR_LN("Implement me!");
}

//------------------------------------------------------------------------------
void GraphicsContext::SetUnorderedAccessView(ObjectHandle h, Color* clearColor)
{
  auto type = h.type();

  ID3D11UnorderedAccessView* view = nullptr;
  if (type == ObjectHandle::kStructuredBuffer)
  {
    StructuredBuffer* buf = GRAPHICS._structuredBuffers.Get(h);
    view = buf->uav.resource;
  }
  else if (type == ObjectHandle::kRenderTarget)
  {
    RenderTargetResource* res = GRAPHICS._renderTargets.Get(h);
    view = res->uav.resource;

    if (clearColor)
    {
      _ctx->ClearRenderTargetView(res->rtv.resource, &clearColor->x);
    }
  }
  else
  {
    LOG_WARN("Trying to set an unsupported UAV type!");
    return;
  }

  u32 initialCount = 0;
  _ctx->CSSetUnorderedAccessViews(0, 1, &view, &initialCount);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetSamplerState(ObjectHandle h, u32 slot, ShaderType shaderType)
{
  ID3D11SamplerState* samplerState = GRAPHICS._sampler_states.Get(h);

  if (shaderType == ShaderType::VertexShader)
    _ctx->VSSetSamplers(slot, 1, &samplerState);
  else if (shaderType == ShaderType::PixelShader)
    _ctx->PSSetSamplers(slot, 1, &samplerState);
  else if (shaderType == ShaderType::ComputeShader)
    _ctx->CSSetSamplers(slot, 1, &samplerState);
  else if (shaderType == ShaderType::GeometryShader)
    _ctx->GSSetSamplers(slot, 1, &samplerState);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetSamplers(
    const ObjectHandle* h,
    u32 slot,
    u32 numSamplers,
    ShaderType shaderType)
{
  vector<ID3D11SamplerState*> samplers;
  for (u32 i = 0; i < numSamplers; ++i)
    samplers.push_back(GRAPHICS._sampler_states.Get(h[i]));

  if (shaderType == ShaderType::VertexShader)
    _ctx->VSSetSamplers(slot, numSamplers, samplers.data());
  else if (shaderType == ShaderType::PixelShader)
    _ctx->PSSetSamplers(slot, numSamplers, samplers.data());
  else if (shaderType == ShaderType::ComputeShader)
    _ctx->CSSetSamplers(slot, numSamplers, samplers.data());
  else if (shaderType == ShaderType::GeometryShader)
    _ctx->GSSetSamplers(slot, numSamplers, samplers.data());
}

//------------------------------------------------------------------------------
void GraphicsContext::SetConstantBuffer(
    ObjectHandle h,
    const void* buf,
    size_t len,
    ShaderType shaderType,
    u32 slot)
{
  ID3D11Buffer *buffer = GRAPHICS._constantBuffers.Get(h);
  D3D11_MAPPED_SUBRESOURCE sub;
  if (SUCCEEDED(_ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub)))
  {
    memcpy(sub.pData, buf, len);
    _ctx->Unmap(buffer, 0);
  }

  if (shaderType == ShaderType::VertexShader)
    _ctx->VSSetConstantBuffers(slot, 1, &buffer);
  else if (shaderType == ShaderType::PixelShader)
    _ctx->PSSetConstantBuffers(slot, 1, &buffer);
  else if (shaderType == ShaderType::ComputeShader)
    _ctx->CSSetConstantBuffers(slot, 1, &buffer);
  else if (shaderType == ShaderType::GeometryShader)
    _ctx->GSSetConstantBuffers(slot, 1, &buffer);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetGpuObjects(const GpuObjects& obj)
{
  // NOTE: We don't check for handle validity here, as setting an invalid (uninitialized)
  // handle is the same as setting NULL, ie unbinding
  SetVertexShader(obj._vs);
  SetGeometryShader(obj._gs);
  SetPixelShader(obj._ps);
  SetLayout(obj._layout);
  SetVertexBuffer(obj._vb);
  SetIndexBuffer(obj._ib);
  SetPrimitiveTopology(obj._topology);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetGpuState(const GpuState& state)
{
  static float blendFactor[4] = { 1, 1, 1, 1 };
  SetRasterizerState(state._rasterizerState);
  SetBlendState(state._blendState, blendFactor, 0xffffffff);
  SetDepthStencilState(state._depthStencilState, 0);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetGpuStateSamplers(const GpuState& state, ShaderType shaderType)
{
  SetSamplers(state._samplers, 0, 4, shaderType);
}

//------------------------------------------------------------------------------
void GraphicsContext::SetViewports(u32 numViewports, const D3D11_VIEWPORT& viewport)
{
  vector<D3D11_VIEWPORT> viewports(numViewports, viewport);
  _ctx->RSSetViewports(numViewports, viewports.data());
}

//------------------------------------------------------------------------------
void GraphicsContext::SetScissorRect(u32 numRects, const D3D11_RECT* rects)
{
  _ctx->RSSetScissorRects(numRects, rects);
}