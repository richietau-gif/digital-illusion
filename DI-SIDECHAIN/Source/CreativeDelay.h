#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace di {
// Stereo delay with continuous feedback tails and gated input send.
// Storage is allocated ONLY in prepare(), never on the audio thread.
// The host controls milliseconds or beat-synchronized duration.
class CreativeDelay {
public:
 void prepare(double sampleRate){
  sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.,192000.):44100.;
  const auto n=static_cast<size_t>(std::ceil(4.0*sr))+4;
  for(auto& b:buffer)b.assign(n,0.f);
  fadeCoeff=1.f-std::exp(-1.f/static_cast<float>(.004*sr));
  reset();
 }
 void reset() noexcept {
  write=0;inputGate=0.f;lastSeconds=-1.;lastTap=1;
  for(auto& b:buffer)std::fill(b.begin(),b.end(),0.f);
 }
 void process(float& left,float& right,bool enabled,float wet,float feedback,
              double seconds,bool pingpong) noexcept{
  if(buffer[0].empty())return;
  const auto n=buffer[0].size();
  // Host controls change at block boundaries in normal use; cache the tap
  // selection instead of converting floating-point times at every sample.
  seconds=std::isfinite(seconds)?std::clamp(seconds,.001,3.99):.25;
  if(seconds!=lastSeconds){
   lastSeconds=seconds;
   lastTap=static_cast<size_t>(std::clamp(
    static_cast<int>(std::round(seconds*sr)),1,static_cast<int>(n)-2));
  }
  const auto rd=(write+n-lastTap)%n;
  const float dl=buffer[0][rd],dr=buffer[1][rd];
  const float fb=std::clamp(std::isfinite(feedback)?feedback:0.f,0.f,.95f);
  const float w=std::clamp(std::isfinite(wet)?wet:0.f,0.f,1.f);
  // Opening a cell must capture its very first transient. Only gate closing
  // gets a short ramp; otherwise one-shot drums disappear into the attack.
  if(enabled)inputGate=1.f;
  else inputGate+=fadeCoeff*(0.f-inputGate);
  const float inL=std::isfinite(left)?left:0.f;
  const float inR=std::isfinite(right)?right:0.f;
  buffer[0][write]=std::clamp(inL*inputGate+fb*(pingpong?dr:dl),-16.f,16.f);
  buffer[1][write]=std::clamp(inR*inputGate+fb*(pingpong?dl:dr),-16.f,16.f);
  if(++write==n)write=0;
  left=inL+dl*w;
  right=inR+dr*w;
 }
private:
 double sr=44100.;
 std::array<std::vector<float>,2> buffer;
 size_t write=0;
 float inputGate=0.f,fadeCoeff=.01f;
 double lastSeconds=-1.;
 size_t lastTap=1;
};
} // namespace di
