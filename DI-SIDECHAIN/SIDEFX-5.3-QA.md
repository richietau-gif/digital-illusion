# DI SIDEFX 5.3 — Windows Release & Ableton QA

## What changed since the tested 5.2B binary
- Six musical GRID tracks instead of eight rows with separate REVERB and
  DISTORT rows. Increased cell height and added miniature duck curve previews.
- Each cell chooses independent DUCK, reverb send (R), distortion send (D),
  pre-reverb REVERSE (<) and envelope BYPASS (X). BYP bypasses *only* the
  duck envelope; the enabled FX sends continue. An inactive cell triggers
  nothing. Turning on a cell FX automatically activates its cell and rack.
- Curve preset selection and copy/paste/undo target the audible independent
  duck track, not the old legacy 32-step shape bank.
- Five modulation lanes: Volume / Filter / Pan / Reverb / Distort.
- Matte graphite/black look, minimal slick rotary arcs, disabled filter
  controls greyed out by relevance and redundant micro-number fields hidden.
- Five reverb tonal profiles with Dry/Wet, Size/Length, Decay and *predelay*.
- Five distortion models with Drive, Dry/Wet, Tone and 2x oversampling.
- MIDI note triggers 36..41 now honor Audio-only trigger setting.
- Cutoff controls widened and frequency taper adjusted for higher-frequency
  modern sound design. Values restricted safely by filter DSP.

## Automated Windows build
Open:
https://github.com/richietau-gif/digital-illusion/actions/workflows/build-vst3.yml

Choose branch `feature/di-sidechain-v5.2b-grid` and select **Run workflow**.
The workflow also declares `push` for the development branch; depending on
GitHub Actions/permissions/quota, only manual workflow_dispatch may run.

Expected Windows x64 Release:
- CMake project `DI_SIDEFX` version 5.3.0
- Complete VST3 bundle `DI SIDEFX.vst3`
- Strict pass on all CTest smoke tests and pluginval
- Artifact `DI-SIDEFX-5.3-Windows-x64`, containing
  `DI-SIDEFX-5.3-Windows-x64.zip`

Do not rename the old 5.2B file or claim the new version is available until
its exact build SHA has passed.

## Manual Ableton Live 12 Suite tests
- [ ] Install DI SIDEFX 5.3 VST3 without deleting DI SIDECHAIN
- [ ] Presets: Classic Duck, Deep Pump, Breakbeat Pump and shape shortcuts
- [ ] Compare wave curve editor to audible duck processing for Tracks 1–6
- [ ] Verify grid 1/4/8/16-step loops at 85, 122, 145 BPM, with seeking
- [ ] Enable and disable R, D, REV, BYP per cell; preserve via host save/reload
- [ ] Switch scenes A–D and copy scenes
- [ ] Reverb tail continues after send closes; no permanent dry attenuation
- [ ] Reverse before reverb creates causal rising preverb on musical slices
- [ ] Distortion Tone changes high-frequency character, predelay delays wet only
- [ ] Filter Full/Low/High/Band knobs grey out appropriately
- [ ] Check UI at 50%, 75%, 100%; no clipped labels or tiny sliders
- [ ] Windows CTest, pluginval strictness 5, and ZIP artifact pass

**Status:** source implementation and static checks only; binary 5.3 is
not certified until the Windows CI + pluginval and manual Ableton validation.
