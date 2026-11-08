# DI SIDECHAIN 4.1 — Ableton Live 12 validation

Use a **copy** of an Ableton project. Do not replace an installed stable VST3 until these checks pass.

## Build and automated tests (Windows 11)

1. Configure with CMake/Visual Studio 2022 x64 and `-DBUILD_TESTING=ON`.
2. Build Release; run `ctest --test-dir build/di-sidechain -C Release --output-on-failure`.
3. Require all six tests to pass: TriggerLogic, ShapeSequencer, FxWaveshaper, PeakDetector, OversampledFx2x, FilterCutoff.
4. Confirm the `DI SIDECHAIN.vst3` bundle exists and Ableton scans it without crashing.
5. Compare VST3 plugin latency reported to Ableton against the expected **14 samples** (two independent 2x FX stages).

## Audio and timing

- [ ] Load on a stereo audio track; no crash on insert, editor open/close, project save/reopen.
- [ ] Disable FX: dry output remains aligned with 14-sample reported latency; verify with an impulse/null comparison.
- [ ] Sweep FX Mix from 0% to 100%, Drive 0–24 dB, and the legacy Saturator ↔ Soft Clipper type while playing. Check separate **FX** and **SATURATOR** toggles in all four on/off combinations. Listen for clicks, phase changes and silence.
- [ ] Feed external kick audio through Ableton's sidechain routing; verify Audio only, MIDI only and Audio + MIDI modes independently.
- [ ] Test rapid kicks inside and outside trigger holdoff; no delayed ghost triggers.
- [ ] Send MIDI note-ons while sidechain audio plays; one restart per event/sample, no stuck envelope.
- [ ] Enable 16-step sequencer. Check independent Volume, Filter and Pan lanes; save and reopen all three.
- [ ] Tempo sync at 90, 120, 174 BPM; change tempo during playback; sequence follows Ableton's PPQ timeline.
- [ ] Loop a 1-bar and 2-bar region; seek backward and forward; stop/start transport; step position should follow the host.
- [ ] Switch 44.1 / 48 / 96 kHz and buffer sizes 64 / 128 / 512; no crash or filter instability.
- [ ] Automate filter cutoff, mode, FX Drive/Mix, shape offset, sequencer step count and sync while playing.
- [ ] Confirm preset restore, Undo/Redo, Copy/Paste/Reset, GUI resize/reopen, and no missing controls.
- [ ] Use SETUP gear: verify menu is hidden by default, overlays the plugin only while open, and dismisses when clicking elsewhere.
- [ ] Change GUI scaling between 100%, 75% and 50%; verify all controls remain aligned/clickable after repeated changes, project reopen, and user preset import.
- [ ] Save/load user .dipreset file and verify 16-step shapes, modulation curves, active FX/SATURATOR states, and editor preferences persist.
- [ ] Verify Economy/Standard/High curve rendering and check 2x oversampling is accurately labeled (no unimplemented 4x processing claim).
- [ ] Check 16 steps and reduced step counts: selected step must stay within the current range.
- [ ] Check CPU use with multiple instances and confirm no audio-thread allocations in a profiler.

## Release gate

Do not merge PR #12 or label the build production-ready until the Windows Release build,
all six CTest tests, plugin scan, and manual Ableton audio/GUI checks pass.
GitHub Actions is **manual-only** to preserve the remaining CI minutes.
