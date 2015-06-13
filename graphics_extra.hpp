#pragma once
#include "object_handle.hpp"

namespace tano
{
  class GraphicsContext;

  //------------------------------------------------------------------------------
  void VertexFlagsToLayoutDesc(u32 vertexFlags, vector<D3D11_INPUT_ELEMENT_DESC>* desc);
  void SetClientSize(HWND hwnd, int width, int height);
  bool InitConfigDialog(HINSTANCE hInstance);

  //------------------------------------------------------------------------------
  enum class ShaderType
  {
    VertexShader,
    PixelShader,
    GeometryShader,
    ComputeShader,
  };

  //------------------------------------------------------------------------------
  struct BufferFlag
  {
    enum Enum
    {
      CreateMipMaps        = 1 << 0,
      CreateDepthBuffer    = 1 << 1,
      CreateSrv            = 1 << 2,
      CreateUav            = 1 << 3,
    };

    struct Bits
    {
      u32 mipMaps : 1;
      u32 depthBuffer : 1;
      u32 srv : 1;
      u32 uav : 1;
    };
  };

  typedef Flags<BufferFlag> BufferFlags;

  //------------------------------------------------------------------------------
  struct VideoAdapter
  {
    CComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC desc;
    vector<DXGI_MODE_DESC> displayModes;
  };

  //------------------------------------------------------------------------------
  struct Setup
  {
    CComPtr<IDXGIFactory> dxgi_factory;
    vector<VideoAdapter> videoAdapters;
    int selectedAdapter = -1;
    int SelectedDisplayMode = -1;
    int selectedAspectRatio = -1;
    int multisampleCount = -1;
    int width = -1;
    int height = -1;
    bool windowed = false;
  };

  //------------------------------------------------------------------------------
  template<class Resource, class Desc>
  struct PointerAndDesc
  {
    void Release()
    {
      ptr.Release();
    }
    CComPtr<Resource> ptr;
    Desc desc;
  };

  //------------------------------------------------------------------------------
  struct DepthStencilResource
  {
    DepthStencilResource()
    {
      Reset();
    }

    void Reset()
    {
      texture.Release();
      view.Release();
    }

    PointerAndDesc<ID3D11Texture2D, D3D11_TEXTURE2D_DESC> texture;
    PointerAndDesc<ID3D11DepthStencilView, D3D11_DEPTH_STENCIL_VIEW_DESC> view;
  };

  //------------------------------------------------------------------------------
  struct RenderTargetResource
  {
    RenderTargetResource()
    {
      Reset();
    }

    void Reset()
    {
      texture.Release();
      view.Release();
      srv.Release();
      uav.Release();
    }

    bool inUse = true;
    PointerAndDesc<ID3D11Texture2D, D3D11_TEXTURE2D_DESC> texture;
    PointerAndDesc<ID3D11RenderTargetView, D3D11_RENDER_TARGET_VIEW_DESC> view;
    PointerAndDesc<ID3D11ShaderResourceView, D3D11_SHADER_RESOURCE_VIEW_DESC> srv;
    PointerAndDesc<ID3D11UnorderedAccessView, D3D11_UNORDERED_ACCESS_VIEW_DESC> uav;
  };

  //------------------------------------------------------------------------------
  struct TextureResource
  {
    void Reset()
    {
      texture.Release();
      view.Release();
    }
    PointerAndDesc<ID3D11Texture2D, D3D11_TEXTURE2D_DESC> texture;
    PointerAndDesc<ID3D11ShaderResourceView, D3D11_SHADER_RESOURCE_VIEW_DESC> view;
  };

  //------------------------------------------------------------------------------
  struct SimpleResource
  {
    void Reset()
    {
      resource.Release();
      view.Release();
    }
    CComPtr<ID3D11Resource> resource;
    PointerAndDesc<ID3D11ShaderResourceView, D3D11_SHADER_RESOURCE_VIEW_DESC> view;
  };

  //------------------------------------------------------------------------------
  struct StructuredBuffer
  {
    PointerAndDesc<ID3D11Buffer, D3D11_BUFFER_DESC> buffer;
    PointerAndDesc<ID3D11ShaderResourceView, D3D11_SHADER_RESOURCE_VIEW_DESC> srv;
    PointerAndDesc<ID3D11UnorderedAccessView, D3D11_UNORDERED_ACCESS_VIEW_DESC> uav;
  };

  //------------------------------------------------------------------------------
  struct SwapChain
  {
    SwapChain(const char* name) : _name(name) {}
    bool CreateBackBuffers(u32 width, u32 height, DXGI_FORMAT format);
    void Present();

    string _name;
    HWND _hwnd;
    CComPtr<IDXGISwapChain> _swapChain;
    DXGI_SWAP_CHAIN_DESC _desc;
    ObjectHandle _renderTarget;
    ObjectHandle _depthStencil;
    CD3D11_VIEWPORT _viewport;
    u32 _width, _height;
  };

  //------------------------------------------------------------------------------
  struct ScopedRenderTarget
  {
    ScopedRenderTarget(
        int width,
        int height,
        DXGI_FORMAT format,
        const BufferFlags& bufferFlags = BufferFlags(BufferFlag::CreateSrv));
    ScopedRenderTarget(
        DXGI_FORMAT format,
        const BufferFlags& bufferFlags = BufferFlags(BufferFlag::CreateSrv));
    ~ScopedRenderTarget();

    int _width, _height;
    DXGI_FORMAT _format;
    ObjectHandle _rtHandle;
    ObjectHandle _dsHandle;
  };

  //------------------------------------------------------------------------------
  u32* GenerateQuadIndices(u32 numQuads, u32* ibSize);

  //------------------------------------------------------------------------------
  void InitDefaultDescs();
  extern CD3D11_BLEND_DESC blendDescBlendSrcAlpha;
  extern CD3D11_BLEND_DESC blendDescBlendOneOne;
  extern CD3D11_BLEND_DESC blendDescPreMultipliedAlpha;
  
  extern CD3D11_RASTERIZER_DESC rasterizeDescCullNone;

  extern CD3D11_DEPTH_STENCIL_DESC depthDescDepthDisabled;
  extern CD3D11_DEPTH_STENCIL_DESC depthDescDepthWriteDisabled;
}