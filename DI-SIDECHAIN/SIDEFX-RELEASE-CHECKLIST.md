# DI SIDEFX 5.2B — Windows VST3 Release Gate

This branch is a **new plugin**, with a unique VST3 identity (DiFx) and its own
`DI SIDEFX.vst3` bundle. It is not an in-place upgrade to DI SIDECHAIN.

## Build the Windows ZIP

From a Windows 11 workstation with Visual Studio 2022 C++ and CMake installed:

```powershell
git checkout feature/di-sidechain-v5.2b-grid
powershell -ExecutionPolicy Bypass -File .\DI-SIDECHAIN\build-sidefx-windows.ps1
```

Optional pluginval validation:

```powershell
powershell -ExecutionPolicy Bypass -File .\DI-SIDECHAIN\build-sidefx-windows.ps1 -PluginvalExe "C:\path\to\pluginval.exe"
```

Output (only after a successful local build):
`build/di-sidefx-local/packages/DI-SIDEFX-5.2B-Windows-x64.zip`

GitHub workflow: `.github/workflows/di-sidefx-5.2b-windows.yml`
(Expected artifact: `DI-SIDEFX-5.2B-Windows-x64`.)

Note: A GitHub branch commit alone is **not** a compiled VST3 or proof that
GitHub Actions actually ran. Do not offer a download until an actual artifact
has passed checks and exists.

## Installation

- Close Ableton Live 12.
- Extract the ZIP. Install the complete `DI SIDEFX.vst3` bundle into
  `C:\Program Files\Common Files\VST3\`.
- Leave `DI SIDECHAIN.vst3` intact; the two plugin identities are distinct.
- Start Ableton, enable VST3 scanning and rescan plugins.
- Insert DI SIDEFX on a stereo audio track with no other mastering limiter
  during initial inspection.

## Minimum pre-release checks

- [ ] Windows 2022 x64 Release compile succeeds
- [ ] All CTest smoke tests pass (including DuckVoice, ShapeSequencer, creative
      reverb/reverse, oversampled distortion and scene model)
- [ ] pluginval strictness 5 passes
- [ ] No crash opening/closing, resizing or switching 50/75/100% GUI
- [ ] Four scenes survive save/close/reopen and copy scene preserves reverse flags
- [ ] Patterns of 1, 4, 8 and 16 steps retrigger correctly through Ableton loops
- [ ] MIDI 36–41 triggers distinct duck tracks with velocity response
- [ ] Audio sidechain input still triggers correctly
- [ ] Two overlapping duck voices produce strongest-wins attenuation, never
      unbounded gain reduction
- [ ] REVERB reverse `<` works per-cell and tails continue after a gated step
- [ ] Fast tempo changes and manual seeking do not produce out-of-range access
- [ ] Distortion 2x output, reported plugin latency, and dry/wet alignment checked
- [ ] No clicks at FX step boundaries; no sustained DC or nonfinite audio
- [ ] CPU / memory profiled at 44.1/48/96 kHz, 64–1024 host buffer sizes
- [ ] ZIP installed and both DI SIDECHAIN and DI SIDEFX visible side by side

## Known quality caveats

The reverb models are five algorithmic profiles of one JUCE/FreeVerb-style
core; Length and Decay are not independently calibrated RT60 controls.
Reverse is causal, buffering the previous synchronized musical slice rather
than predicting future audio. The current preview has not been certified by
pluginval or auditioned in Ableton Live.

## Starting the existing GitHub Actions workflow manually

The repository default branch already registers `.github/workflows/build-vst3.yml`.
The **development-branch revision** of that same workflow has now been adapted
for DI SIDEFX, so it can be selected without merging to main.

1. Open https://github.com/richietau-gif/digital-illusion/actions/workflows/build-vst3.yml
2. Select **Run workflow**, choose branch `feature/di-sidechain-v5.2b-grid`, and run.
3. Inspect the Windows Release, CTest and pluginval checks.
4. If successful, download artifact `DI-SIDEFX-5.2B-Windows-x64`.

If Run workflow is missing, check repository Actions permissions, billing / quota,
and whether manual dispatch remains enabled. A workflow present only on a non-default
branch may not appear in the Actions workflow list: the existing default-registered
workflow path avoids this limitation.
