#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

// Six independent, allocation-free duck voices. Events are triggered only on
// step ENTER, not repeatedly while a step is held. The strongest attenuation
// wins, avoiding the unwanted level collapse of multiplying six gains.
namespace di {
class DuckVoiceEngine {
public:
 static constexpr int tracks=6, nodes=8;
 using Shape=std::array<float,nodes>;
 struct TrackSettings {
  float depth=1.f;          // 0 = no duck, 1 = full programmed shape
  float duration=1.f;       // multiple of one host step, 0.05 .. 4
  Shape shape{{0.f,.02f,.2f,.65f,.9f,1.f,1.f,1.f}};
 };
 void reset() noexcept {
  lastGeneration=std::numeric_limits<std::uint64_t>::max();
  for(auto& v:voices)v=Voice{};
 }
 void setTrack(int track,TrackSettings settings) noexcept {
  if(track<0||track>=tracks)return;
  settings.depth=safe(settings.depth,0.f,1.f,1.f);
  settings.duration=safe(settings.duration,.05f,4.f,1.f);
  for(auto& p:settings.shape)p=safe(p,0.f,1.f,1.f);
  tracksData[static_cast<size_t>(track)]=settings;
 }
 TrackSettings getTrack(int track) const noexcept {
  return track<0||track>=tracks?TrackSettings{}:tracksData[static_cast<size_t>(track)];
 }
 // Called once per audio sample with shared Ableton step position.
 // active[track] is a snapshot of the currently entered column's cells.
 void triggerTrack(int track,double samplesPerStep,float velocity=1.f) noexcept {
  if(track<0||track>=tracks)return;
  if(!std::isfinite(samplesPerStep)||samplesPerStep<=0.)return;
  const auto& settings=tracksData[static_cast<size_t>(track)];
  auto& voice=voices[static_cast<size_t>(track)];
  voice.age=0.;
  voice.samples=std::max(1.,samplesPerStep*settings.duration);
  voice.depth=settings.depth*safe(velocity,0.f,1.f,1.f);
  voice.shape=settings.shape;
  voice.active=true;
 }
 // Deterministic convenience entry point for standalone tests.
 float process(int step,const std::array<bool,tracks>& active,double samplesPerStep) noexcept {
  return process(step,active,samplesPerStep,static_cast<std::uint64_t>(std::max(0,step)));
 }
 float process(int step,const std::array<bool,tracks>& active,double samplesPerStep,std::uint64_t generation) noexcept {
  (void)step;
  if(generation!=lastGeneration){
   lastGeneration=generation;
   for(int t=0;t<tracks;++t){
    if(!active[static_cast<size_t>(t)])continue;
    triggerTrack(t,samplesPerStep,1.f);
   }
  }
  float result=1.f;
  for(auto& v:voices){
   if(!v.active)continue;
   const double pos=std::clamp(v.age/v.samples,0.,1.);
   const float curve=interpolate(v.shape,static_cast<float>(pos));
   const float gain=1.f-v.depth*(1.f-curve);
   result=std::min(result,std::clamp(gain,0.f,1.f));
   v.age+=1.;
   if(v.age>=v.samples)v.active=false;
  }
  return result;
 }
 static float interpolate(const Shape& s,float t) noexcept {
  const float x=std::clamp(t,0.f,1.f)*7.f;
  const int a=std::min(6,static_cast<int>(x));
  float mix=x-a;
  mix=mix*mix*(3.f-2.f*mix);
  return s[static_cast<size_t>(a)]+(s[static_cast<size_t>(a+1)]-s[static_cast<size_t>(a)])*mix;
 }
private:
 struct Voice {
  Shape shape{};
  double age=0.,samples=1.;
  float depth=0.f;
  bool active=false;
 };
 static float safe(float v,float low,float high,float fallback) noexcept {
  return std::isfinite(v)?std::clamp(v,low,high):fallback;
 }
 std::array<Voice,tracks> voices{};
 std::array<TrackSettings,tracks> tracksData{};
 std::uint64_t lastGeneration=std::numeric_limits<std::uint64_t>::max();
};
} // namespace di
