#pragma once

#include "../base_effect.hpp"
#include "../gpu_objects.hpp"
#include "../generated/demo.types.hpp"

namespace tano
{
  class RayMarcher : public BaseEffect
  {
  public:

    RayMarcher(const string &name, u32 id);
    ~RayMarcher();
    virtual bool Init(const char* configFile) override;
    virtual bool Update(const UpdateState& state) override;
    virtual bool Render() override;
    virtual bool Close() override;

    static const char* Name();
    static BaseEffect* Create(const char* name, u32 id);
    static void Register();

  private:

    void Reset();
#if WITH_IMGUI
    void RenderParameterSet();
    void SaveParameterSet();
#endif

    struct CBufferPerFrame
    {
      Vector2 dim;
      float time;
    };

    GpuState _raymarcherState;
    GpuObjects _raymarcherGpuObjects;

    ConstantBuffer<CBufferPerFrame> _cbPerFrame;
    RayMarcherSettings _settings;
    string _configName;
  };
}
