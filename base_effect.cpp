
#include "base_effect.hpp"
#include "graphics.hpp"

using namespace tano;

//------------------------------------------------------------------------------
BaseEffect::BaseEffect(const string& instanceName, u32 id)
  : _instanceName(instanceName)
  , _id(id)
  , _running(false)
  , _ctx(GRAPHICS.GetGraphicsContext())
  , _firstTick(true)
{
}

//------------------------------------------------------------------------------
bool BaseEffect::Show()
{
  return true;
}

//------------------------------------------------------------------------------
bool BaseEffect::Hide()
{
  return true;
}

//------------------------------------------------------------------------------
bool BaseEffect::Init(const char* configFile)
{
  return true;
}

//------------------------------------------------------------------------------
bool BaseEffect::InitAnimatedParameters()
{
  return true;
}

//------------------------------------------------------------------------------
bool BaseEffect::Update(const UpdateState& state)
{
  return true;
}

//------------------------------------------------------------------------------
bool BaseEffect::Render()
{
  return true; 
}

//------------------------------------------------------------------------------
bool BaseEffect::Close()
{
  return true; 
}

//------------------------------------------------------------------------------
void BaseEffect::SetDuration(TimeDuration startTime, TimeDuration endTime)
{
  _startTime = startTime;
  _endTime = endTime;
}

//------------------------------------------------------------------------------
void BaseEffect::SetStartTime(TimeDuration startTime)
{
  _startTime = startTime;
}

//------------------------------------------------------------------------------
bool BaseEffect::Running() const
{
  return _running; 
}

//------------------------------------------------------------------------------
void BaseEffect::SetRunning(bool b)
{ 
  _running = b; 
}