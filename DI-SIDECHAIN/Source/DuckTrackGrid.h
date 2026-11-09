#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

// Standalone, realtime-safe scene/track/step pattern data.
// It deliberately does not process audio yet; its fixed-size layout is a
// foundation for a future independent duck-envelope voice scheduler.
namespace di {
class DuckTrackGrid {
public:
 static constexpr int maxScenes=4, maxTracks=8, maxSteps=16;
 struct Cell {
  bool enabled=false;
  float velocity=1.f;   // duck depth multiplier, 0..1
  float length=1.f;     // fraction of step, 0..1
  float shift=0.f;      // signed fraction of step, -0.5..0.5
 };
 struct Track {
  bool muted=false;
  float gain=1.f;
  std::array<Cell,maxSteps> cells{};
 };
 struct Scene {
  int steps=maxSteps;
  std::array<Track,maxTracks> tracks{};
 };
 DuckTrackGrid() noexcept { clear(); }
 void clear() noexcept {
  for(auto& scene:scenes) {
   scene=Scene{};
   for(auto& track:scene.tracks) track=Track{};
  }
  selectedScene=0;
 }
 void selectScene(int index) noexcept {selectedScene=std::clamp(index,0,maxScenes-1);}
 int sceneIndex() const noexcept {return selectedScene;}
 void setStepCount(int count) noexcept {scenes[selectedScene].steps=std::clamp(count,1,maxSteps);}
 int stepCount() const noexcept {return scenes[selectedScene].steps;}
 void setMuted(int track,bool muted) noexcept {
  if(validTrack(track))scenes[selectedScene].tracks[track].muted=muted;
 }
 bool muted(int track) const noexcept {
  return validTrack(track)&&scenes[selectedScene].tracks[track].muted;
 }
 void setCell(int track,int step,Cell cell) noexcept {
  if(!validTrack(track)||!validStep(step))return;
  cell.velocity=sanitize(cell.velocity,0.f,1.f,1.f);
  cell.length=sanitize(cell.length,0.f,1.f,1.f);
  cell.shift=sanitize(cell.shift,-.5f,.5f,0.f);
  scenes[selectedScene].tracks[track].cells[step]=cell;
 }
 Cell getCell(int track,int step) const noexcept {
  if(!validTrack(track)||!validStep(step))return {};
  return scenes[selectedScene].tracks[track].cells[step];
 }
 bool triggers(int track,int step) const noexcept {
  if(!validTrack(track)||step<0||step>=stepCount())return false;
  const auto& t=scenes[selectedScene].tracks[track];
  return !t.muted&&t.cells[step].enabled&&t.cells[step].velocity>0.f;
 }
 // All tracks evaluate one shared song-position step, unlike three unrelated
 // modulation selectors. Each track can schedule its own duck event later.
 uint8_t triggerMask(int step) const noexcept {
  uint8_t mask=0;
  for(int track=0;track<maxTracks;++track)
   if(triggers(track,step))mask=static_cast<uint8_t>(mask|(1u<<track));
  return mask;
 }
 void copyScene(int from,int to) noexcept {
  if(from>=0&&from<maxScenes&&to>=0&&to<maxScenes)scenes[to]=scenes[from];
 }
 const Scene& scene(int index) const noexcept {return scenes[std::clamp(index,0,maxScenes-1)];}
private:
 static constexpr bool validTrack(int i) noexcept {return i>=0&&i<maxTracks;}
 static constexpr bool validStep(int i) noexcept {return i>=0&&i<maxSteps;}
 static float sanitize(float v,float lo,float hi,float fallback) noexcept {
  return std::isfinite(v)?std::clamp(v,lo,hi):fallback;
 }
 std::array<Scene,maxScenes> scenes{};
 int selectedScene=0;
};
} // namespace di
