#pragma once

#include "../base_effect.hpp"
#include "../gpu_objects.hpp"
#include "../generated/demo.types.hpp"
#include "../text_writer.hpp"
#include "../animation_helpers.hpp"
#include "../append_buffer.hpp"
#include "../tano_math.hpp"

namespace tano
{
  namespace scheduler
  {
    struct TaskData;
  }

  class ParticleTunnel : public BaseEffect
  {
  public:

    ParticleTunnel(const string &name, u32 id);
    ~ParticleTunnel();
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

    void UpdateCameraMatrix();
    void GenRandomPoints(float kernelSize);

    struct ParticleType
    {
      V4 pos;
      V4 data;
    };

    struct ParticleEmitter
    {
      void Create(const V3& center, int numParticles);
      void Destroy();
      void Update(float dt);
      void CreateParticle(int idx, float s);
      void CopyToBuffer(ParticleType* vtx);

      // TODO: test speed diff if these are double buffered, to avoid LHS
      // NOTE: this is just trams.. let's make these guys XMVECTORs first
      float* x = nullptr;
      float* y = nullptr;
      float* z = nullptr;
      float* vx = nullptr;
      float* vy = nullptr;
      float* vz = nullptr;

      float* scale = nullptr;

      struct Lifetime
      {
        int total;
        int left;
      };
      Lifetime* _lifetime = nullptr;

      int* _deadParticles = nullptr;

      int _numParticles = 0;
      V3 _center = { 0, 0, 0 };
    };

    struct EmitterKernelData
    {
      ParticleEmitter* emitter;
      float dt;
      int ticks;
      ParticleType* vtx;
    };

    static void UpdateEmitter(const scheduler::TaskData& data);

    struct TextParticles
    {
      TextParticles(const ParticleTunnelSettings& settings) : settings(settings) {}
      void Create(const vector<V3>& verts, float targetTime);
      void Destroy();
      void Update(float seconds, float start, float end);

      float* curX = nullptr;
      float* curY = nullptr;
      float* curZ = nullptr;
      float* startX = nullptr;
      float* startY = nullptr;
      float* startZ = nullptr;
      float* endX = nullptr;
      float* endY = nullptr;
      float* endZ = nullptr;

      int numParticles = 0;
      const ParticleTunnelSettings& settings;
      vector<int> selectedTris;
    };

    SimpleAppendBuffer<ParticleEmitter, 24> _particleEmitters;

    struct CBufferPerFrame
    {
      Matrix world;
      Matrix view;
      Matrix proj;
      Matrix viewProj;
      Color tint;
      Color inner;
      Color outer;
      Vector4 dim;
      Vector4 viewDir;
      Vector4 camPos;
      Vector4 dofSettings;
      Vector4 time;
    };
    ConstantBuffer<CBufferPerFrame> _cbPerFrame;

    struct CBufferBasic
    {
      Matrix world;
      Matrix view;
      Matrix proj;
      Matrix viewProj;
      Vector4 cameraPos;
      Vector4 dim;
    };
    ConstantBuffer<CBufferBasic> _cbBasic;

    string _configName;

    GpuBundle _backgroundBundle;

    ObjectHandle _particleTexture;
    GpuBundle _particleBundle;

    ObjectHandle _lineTexture;
    GpuBundle _lineBundle;
    u32 _numLinesPoints = 0;

    GpuBundle _compositeBundle;

    ParticleTunnelSettings _settings;

    TextWriter _textWriter;
    struct TextData
    {
      vector<V3> outline;
      vector<V3> cap;
      vector<V3> verts;
      vector<V3> transformedVerts;
      vector<int> indices;
      int* neighbours;
    };
    TextData _textData[3];

    TextParticles _neuroticaParticles;
    TextParticles _radioSilenceParticles;
    TextParticles _partyParticles;
    float _particlesStart, _particlesEnd;

    TextParticles* _curParticles = nullptr;

    AnimatedFloat _beatTrack;

    SimpleAppendBuffer<V3, 1024> _randomPoints;
    GpuBundle _plexusLineBundle;

  };
}
