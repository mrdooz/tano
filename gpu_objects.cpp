#include "gpu_objects.hpp"
#include "graphics.hpp"
#include "graphics_context.hpp"
#include "init_sequence.hpp"

using namespace tano;
using namespace bristol;

//------------------------------------------------------------------------------
bool GpuObjects::CreateDynamic(
  u32 ibSize, DXGI_FORMAT ibFormat,
  u32 vbSize, u32 vbElemSize)
{
  return CreateDynamicIb(ibSize, ibFormat, nullptr) && CreateDynamicVb(vbSize, vbElemSize, nullptr);
}

//------------------------------------------------------------------------------
bool GpuObjects::CreateDynamic(
    u32 ibSize, DXGI_FORMAT ibFormat, const void* ibData,
    u32 vbSize, u32 vbElemSize, const void* vbData)
{
  return CreateDynamicIb(ibSize, ibFormat, ibData) && CreateDynamicVb(vbSize, vbElemSize, vbData);
}

//------------------------------------------------------------------------------
bool GpuObjects::CreateDynamicVb(u32 vbSize, u32 vbElemSize, const void* vbData)
{
  _vbSize = vbSize;
  _vbElemSize = vbElemSize;
  _numVerts = _vbSize / _vbElemSize;
  _vb = GRAPHICS.CreateBuffer(D3D11_BIND_VERTEX_BUFFER, vbSize, true, vbData, vbElemSize);
  return _vb.IsValid();
}

//------------------------------------------------------------------------------
bool GpuObjects::CreateDynamicIb(u32 ibSize, DXGI_FORMAT ibFormat, const void* ibData)
{
  _ibSize = ibSize;
  _ibFormat = ibFormat;
  _numIndices = _ibSize / IndexSizeFromFormat(ibFormat);
  _ib = GRAPHICS.CreateBuffer(D3D11_BIND_INDEX_BUFFER, ibSize, true, ibData, ibFormat);
  return _ib.IsValid();
}

//------------------------------------------------------------------------------
bool GpuObjects::CreateVertexBuffer(u32 vbSize, u32 vbElemSize, const  void* vbData)
{
  _vbSize = vbSize;
  _vbElemSize = vbElemSize;
  _numVerts = _vbSize / _vbElemSize;
  _vb = GRAPHICS.CreateBuffer(D3D11_BIND_VERTEX_BUFFER, vbSize, false, vbData, vbElemSize);
  return _vb.IsValid();
}

//------------------------------------------------------------------------------
bool GpuObjects::CreateIndexBuffer(u32 ibSize, DXGI_FORMAT ibFormat, const  void* ibData)
{
  _ibSize = ibSize;
  _ibFormat = ibFormat;
  _numIndices = _ibSize / IndexSizeFromFormat(ibFormat);
  _ib = GRAPHICS.CreateBuffer(D3D11_BIND_INDEX_BUFFER, ibSize, false, ibData, ibFormat);
  return _ib.IsValid();
}

//------------------------------------------------------------------------------
bool GpuObjects::LoadShadersFromFile(
    const char* filename,
    const char* vsEntry,
    const char* gsEntry,
    const char* psEntry,
    u32 flags,
    vector<D3D11_INPUT_ELEMENT_DESC>* elements)
{
  ObjectHandle* vs = vsEntry ? &_vs : nullptr;
  ObjectHandle* gs = gsEntry ? &_gs : nullptr;
  ObjectHandle* ps = psEntry ? &_ps : nullptr;
  ObjectHandle* layout = (flags || elements) ? &_layout : nullptr;

  if (flags || elements)
  {
    if (flags)
    {
      vector<D3D11_INPUT_ELEMENT_DESC> desc;
      VertexFlagsToLayoutDesc(flags, &desc);
      return GRAPHICS.LoadShadersFromFile(filename, vs, gs, ps, layout, &desc, vsEntry, gsEntry, psEntry);
    }

    // fix up the offsets on the elements
    u32 ofs = 0;
    map<string, u32> semanticIndex;
    for (D3D11_INPUT_ELEMENT_DESC& e : *elements)
    {
      e.AlignedByteOffset = ofs;
      e.SemanticIndex = semanticIndex[e.SemanticName]++;
      ofs += SizeFromFormat(e.Format);
    }
    return GRAPHICS.LoadShadersFromFile(filename, vs, gs, ps, layout, elements, vsEntry, gsEntry, psEntry);
  }


  return GRAPHICS.LoadShadersFromFile(filename, vs, gs, ps, layout, nullptr, vsEntry, gsEntry, psEntry);
}

//------------------------------------------------------------------------------
bool GpuState::Create(
  const D3D11_DEPTH_STENCIL_DESC* dssDesc,
  const D3D11_BLEND_DESC* blendDesc,
  const D3D11_RASTERIZER_DESC* rasterizerDesc)
{
  _depthStencilState = GRAPHICS.CreateDepthStencilState(
    dssDesc ? *dssDesc : CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT()));

  _blendState = GRAPHICS.CreateBlendState(
    blendDesc ? *blendDesc : CD3D11_BLEND_DESC(CD3D11_DEFAULT()));

  _rasterizerState = GRAPHICS.CreateRasterizerState(
    rasterizerDesc ? *rasterizerDesc : CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT()));

  CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
  _samplers[Linear] = GRAPHICS.CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  _samplers[LinearWrap] = GRAPHICS.CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  _samplers[LinearBorder] = GRAPHICS.CreateSamplerState(samplerDesc);

  samplerDesc.AddressU = samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  _samplers[Point] = GRAPHICS.CreateSamplerState(samplerDesc);

  return
    _depthStencilState.IsValid() && _blendState.IsValid() && _rasterizerState.IsValid() &&
    _samplers[0].IsValid() && _samplers[1].IsValid() && _samplers[2].IsValid() && _samplers[3].IsValid();
}

//------------------------------------------------------------------------------
bool GpuBundle::Create(const BundleOptions& options)
{
  const BundleOptions::OptionFlags& flags = options.flags;
  BEGIN_INIT_SEQUENCE();
  INIT_FATAL(state.Create(
    flags.IsSet(BundleOptions::OptionFlag::DepthStencilDesc) ? &options.depthStencilDesc : nullptr,
    flags.IsSet(BundleOptions::OptionFlag::BlendDesc) ? &options.blendDesc : nullptr,
    flags.IsSet(BundleOptions::OptionFlag::RasterizerDesc) ? &options.rasterizerDesc : nullptr));

  if (options.shaderFile)
  {
    // Because the input elements are modified, we need to save a local copy
    vector<D3D11_INPUT_ELEMENT_DESC> inputElements(options.inputElements);
    INIT(objects.LoadShadersFromFile(
      options.shaderFile,
      options.vsEntry,
      options.gsEntry,
      options.psEntry,
      options.vertexFlags,
      inputElements.empty() ? nullptr : &inputElements));

    objects._topology = options.topology;
  }

  if (flags.IsSet(BundleOptions::OptionFlag::DynamicVb))
  {
    INIT_FATAL(objects.CreateDynamicVb(options.vbNumElems * options.vbElemSize, options.vbElemSize));
  }
  END_INIT_SEQUENCE();
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::DepthStencilDesc(const D3D11_DEPTH_STENCIL_DESC& desc)
{
  depthStencilDesc = desc;
  flags.Set(BundleOptions::OptionFlag::DepthStencilDesc);
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::BlendDesc(const D3D11_BLEND_DESC& desc)
{
  blendDesc = desc;
  flags.Set(BundleOptions::OptionFlag::BlendDesc);
  return *this;

}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::RasterizerDesc(const D3D11_RASTERIZER_DESC& desc)
{
  rasterizerDesc = desc;
  flags.Set(BundleOptions::OptionFlag::RasterizerDesc);
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::ShaderFile(const char* filename)
{
  shaderFile = filename;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::VsEntry(const char* entrypoint)
{
  vsEntry = entrypoint;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::PsEntry(const char* entrypoint)
{
  psEntry = entrypoint;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::GsEntry(const char* entrypoint)
{
  gsEntry = entrypoint;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::CsEntry(const char* entrypoint)
{
  csEntry = entrypoint;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::VertexFlags(u32 flags)
{
  vertexFlags = flags;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::Topology(D3D11_PRIMITIVE_TOPOLOGY topologyIn)
{
  topology = topologyIn;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::InputElements(const vector<D3D11_INPUT_ELEMENT_DESC>& elems)
{
  inputElements = elems;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::DynamicVb(int numElements, int elementSize)
{
  flags.Set(BundleOptions::OptionFlag::DynamicVb);
  vbNumElems = numElements;
  vbElemSize = elementSize;
  return *this;
}

//------------------------------------------------------------------------------
BundleOptions& BundleOptions::DynamicIb(int numElements, int elementSize)
{
  flags.Set(BundleOptions::OptionFlag::DynamicIb);
  ibNumElems = numElements;
  ibElemSize = elementSize;
  return *this;
}
