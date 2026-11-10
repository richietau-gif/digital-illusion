# DI SEQFX 5.4 — Release readiness and Ableton 12 test matrix

## Identity and installation
- Display name / Product: **DI SEQFX**
- JUCE target: `DISeqfx`
- VST3 plug-in code: `DiSq`
- Bundle ID: `com.digitalillusion.diseqfx`
- Standalone bundle: `DI SEQFX.vst3`
- Expected ZIP: `DI-SEQFX-5.4-Windows-x64.zip`
- Can coexist with previous DI SIDEFX and DI SIDECHAIN.

## Compile and automated test gate

Workflow: https://github.com/richietau-gif/digital-illusion/actions/workflows/build-vst3.yml

Choose **Run workflow** and branch `feature/di-sidefx-5.4-editor-delay`.
The workflow uses Visual Studio 2022 x64 Release, then runs CTest, pluginval
strictness 5 and only then creates the ZIP artifact. The workflow is NOT proof
of successful compilation unless the corresponding run and artifact exist.

Portable smoke tests now include:
- `di_dynamic_curve_smoke`: insert, remove, re-position, preset thumbnail shape
- `di_creative_delay_smoke`: stereo timed delay, ping-pong, reset and finite output
- `di_analog_ladder_smoke`: filter resonance and finite processing
- `di_seqfx_layout_smoke`: six tracks + seventh transport row geometry at 50/75/100%
- `di_seqfx_cell_fx_smoke`: all FX bits, envelope-only BYPASS, reverse
- Earlier DuckVoice, ShapeSequencer, 2x Distort, CreativeFx and plugin tests

## Ableton Live 12 manual matrix

1. Insert DI SEQFX on an audio track; check VST3 coexistence with SIDEFX/CHAIN.
2. Default view should be 75%, with no clipped labels, overlapped controls,
   missing sliders or tiny numeric fields.
3. Click GRID ON, then toggle a cell on each of six tracks. A seventh
   SEQUENCE row must align under the same columns. Selecting that row
   must not turn a musical cell on/off.
4. Choose Volume: points and envelope must be GREEN. Drag existing nodes,
   double-click to add, right-click interior points to delete. Test two and
   sixteen nodes plus endpoints; test Undo/Redo and save/reopen.
5. Apply nine curve presets and confirm the thumbnail EXACTLY matches the
   audible editor curve. Copy/paste a 12-point Volume envelope to another
   track and verify point count and timing are preserved.
6. Select the six colored modulation lanes: Volume (lime), Filter (cyan),
   Pan (violet), Reverb (blue), Distort (orange), Delay (pink).
7. Enable an RVB send, then REV; audition pre-reverb reversed swoosh. Confirm
   reverberation tail continues when the input step ends.
8. Select Delay: SYNC division, free milliseconds, Wet, Feedback and Ping-Pong.
   Check the echo continues between active cells but does not ring forever.
9. Test FX routing: a cell with BYP and both RVB/DST sends must skip only
   ducking, preserving effects.
10. Test Filter: FULL/LOW/HIGH/BAND, Clean/Analog Ladder and resonances. Knobs
    not relevant to the current filter mode must appear disabled.
11. Test 10 global presets and 8/12/16-step pattern lengths at 90, 122 and
    145 BPM, including host seeking, loop wrap, and one-step repeat.
12. Test MIDI note 36–41 with velocity, audio sidechain, and trigger mode
    Audio-only or MIDI-only; check behavior at 44.1/48/96 kHz and 64–1024 blocks.
13. Close/reopen UI 20 times, change 50/75/100%, save and restore a DAW
    project, and test A/B snapshot and user preset load/save.

## Known constraints
- The JUCE reverb is a parameterized algorithm with five tonal profiles,
  not five distinct convolution or independently designed reverb engines.
- The reverb decay seconds is an **approximate control**, not measured RT60.
- Reverse pre-effect is buffered causal reversal, not predictive look-ahead.
- Real-world Ableton appearance, sound and performance are not verified by
  source inspection or standalone CTest.

Release status remains **experimental** until Windows VST3 CTest and pluginval
succeed and the actual downloadable artifact can be inspected.
