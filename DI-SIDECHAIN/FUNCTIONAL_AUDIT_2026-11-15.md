# PRECISION SIDECHAIN — functional audit against the approved GUI
Date: 2026-11-15
Branch: `feature/precision-sidechain-1.0`, PR #17
Status: **SOURCE AUDIT + CODE CHANGES, NOT A VERIFIED WINDOWS RELEASE**
Reference: user's approved PRECISION SIDECHAIN render and latest Global VST Host screenshots. The render remains binding for graphic design.

## Acceptance principles
1. A control is **implemented** only when the GUI is connected to an APVTS parameter and the parameter influences real audio or program state. A drawn response line or a passing pluginval cannot prove audible functionality.
2. The 33 currently registered parameters were inspected in `PluginProcessor.cpp` and `PluginEditor.cpp`. Each appears in the editor source, but **appearance alone is not behavioural validation**.
3. No Windows build, CTest, pluginval or Ableton validation was run for these new commits. The previously successful ZIP predates these changes.
4. MIDI/audio/VST3 performance, live host PDC, external sidechain and graphical fidelity require real Windows/Ableton testing.

## GUI ↔ parameter ↔ DSP audit matrix
| GUI / parameter | Source state | Findings and remaining validation |
|---|---|---|
| Shape points, curvature, Undo/Redo | Implemented, partial | Real DSP interpolation, add/move/delete, editor-local undo; inspect cross-thread editing and envelope state reload; true snap-on-release |
| GRID (`editGrid`) | Implemented | Off/1/4/1/8/1/16; grid is for nodes, not rhythmic playback; check 50/75/100% mouse mapping |
| Curve amount (`curveAmount`) | Implemented | Changes interpolation tension; verify waveform audible and state automation |
| Timing OFFSET (`timeOffset`) | Updated | **Full-width bipolar slider directly below graph**, centre 0%, range -100%…+100%, double-click reset, same automatable parameter; small Timing knob no longer displayed |
| Offset DSP | Corrected partially | Delayed negative offset now retains full envelope and idle/no-trigger does not duck; positive offset is a phase advance but not independently guaranteed to anticipate unpredictable triggers without lookahead. Need sample-accurate host/PDC design and tests |
| Length (`length`) / BPM (`sync`, `division`) | Implemented | ms or musical divisions; verify Ableton tempo/time signature, 0 BPM, transport stop/resume |
| Lookahead (`lookahead`) | Implemented, high risk | Delay ring, host PDC updates deferred while transport plays; check dynamic changes after host stops (some hosts suspend processing) and latency accuracy |
| Attack / Release (`attackMs`, `releaseMs`) | Implemented | Two smoothing rates for rising/falling gain; regression needed for no-click transitions |
| Attack Protection (`attackProtection`) | Partial | DSP reads continuous 0–100%, but GUI toggle mainly exposes off/on; reference 80% selector still requires a proper value control |
| Threshold, Holdoff, Detector (`threshold`, `retrigger`, `detector`) | Implemented | Peak/RMS/Transient envelope detection and retrigger holdoff; detector filter/monitor and adjustable hysteresis absent |
| Trigger sources (`triggerMode`, `detectorSource`) | Implemented | Internal/external/auto/MIDI/combined routing exists; UI INT/EXT/MIDI buttons and dropdown need synchronisation/automation QA |
| Gain depth/mix/output (`depth`, `mix`, `output`) | Implemented | Delayed dry/wet alignment, output gain and gain reduction; dedicated dry/wet/bypass buffer tests added |
| Filter power & envelope (`filterEnabled`, `filterEnvelopeEnabled`) | Implemented, partial | Envelope toggle switches dynamic cutoff; filter remains active with envelope off. New actual-buffer A/B test added. Some named creative models are not independent high-quality designs |
| Filter model/cutoff/resonance/drive/mix (`filterModel`, `cutoff`, `resonance`, `filterDrive`, `filterMix`, `filterMod`) | Implemented, partial | LP/HP slope paths, BP/Notch and comb feedback; response curve is an approximation; mode change clicks/automation at high Q not validated; true ladder/formant sound design is outstanding |
| Color power/model/drive/tone/mix (`colorEnabled`, `colorModel`, `colorDrive`, `colorTone`, `colorMix`) | Implemented, partial | Saturation/clip/fold/crush branches affect output; bipolar Tone now brightens for positive values and damps highs for negative; oversampling and anti-aliasing still missing |
| Factory programs and saved state | Partial | 32 programs can change parameters and nodes; zero offset is now the default for normal presets; not every control is set per factory program and presets require quality/categorical review |
| A/B snapshots / Copy A→B | Partial | Editor-session-local, not fully host-persistent; no proof of undo and restoration through DAW re-open |
| User presets | Partial | Save and load via async chooser, but no indexed user preset browser; automation, malformed-file and roundtrip validation outstanding |
| Meters ENV / L / R | Implemented, unvalidated | Atomic reduction and output peak values; response timing/decay and mono/stereo presentation require host QA |
| GUI scale (`uiScale`) | Implemented | 50/75/100%, default 75% / 825×555; compare actual screenshot and event hitboxes against reference at each scale |
| Global bypass (`bypass`) | Implemented | Delayed but unprocessed path; test automation and host bypass/PDC under transport |
| COLOR/FILTER display | Partial | Filter display is a rough analytical guide, *not* measured response for nonlinear/comb/formant types |

## Corrections in this pass
- Replaced Timing Offset rotary with a long symmetrically filled horizontal slider under the complete graph and centre-zero labels; existing parameter ID and DAW automation preserved.
- Factory default offset changed to 0%; specifically designed shifted factory sounds may use nonzero values.
- Fixed end-of-curve truncation for negative offsets, ensured no ducking without a trigger, **and preserved the untouched signal before a negatively shifted envelope actually starts**. The cyan timing preview uses this same gate.
- Centralized offset lifecycle, clamping, and dry/wet alignment in production `PrecisionTiming.h`; enlarged the timing regression to exercise the **same production helpers**, including negative offsets and no-trigger behaviour.
- Added a JUCE-backed CTest target compiling the real processor/editor source, exercising stereo buffers, 44.1/48/96 kHz, bypass transparency, dry/wet identity, all 13 filter selection paths, all six Color models, MIDI triggering, state roundtrips, 32 programs, and distinct audible output with Filter ENV/Color switches.
- Replaced `assert` with release-safe CHECK macros so tests actually execute under Windows Release CI.
- Corrected Color Tone direction so positive values emphasize highs and negative values reduce them.

## Competitor research (not source-derived)
**Devious Machines Duck 2**, official manual:
https://deviousmachines.com/manuals/duck2/html/offset-control.html
https://deviousmachines.com/manuals/duck2/html/trigger-modes.html
https://deviousmachines.com/manuals/duck2/html/whats-new-in-duck-2.html
- Separates shape editing from timing offset, supports ms/sample views and uses PDC/lookahead for early triggered envelopes; also has hysteresis, auditionable trigger filtering, sidechain waveform, variable shapes, undo/redo, editable grid and advanced preset handling.
**Cableguys ShaperBox 3**, official documentation:
https://www.cableguys.com/downloads/Cableguys-ShaperBox-3-Manual.pdf
- VolumeShaper offers precise shaped volume ducking and overlays external sidechain for alignment; extensive filter and envelope routing options.
**Cableguys Kickstart 2**, official manual:
https://downloads.cableguys.com/Nicky-Romero-Kickstart-2-Manual.pdf
- Fast curve-selection workflow, synchronized/MIDI/audio triggers, oscilloscopic comparison and anti-click smoothing.

### Prioritized recommendations
1. **P0:** Prove Windows JUCE build and all three CTest suites; pluginval strictness 5 and ZIP; test crash reproduction in Ableton. Do not declare verified beforehand.
2. **P0:** Proper positive offset/lookahead causality, sample-aligned MIDI/audio and DAW PDC; address suspended-host edge cases and transport automation.
3. **P0:** Real mouse/GUI acceptance at 75%, grid snapping, shift curvature, offset rail, resize, editor open/close.
4. **P1:** Add envelope source and trigger oscilloscope, detector-side filter with audition and adjustable hysteresis, useful for kick/snare isolation.
5. **P1:** Persist A/B and full user-preset library; implement genuine ladder/formant models and add oversampling.
6. **P2:** Non-standard meter/time signatures, velocity-sensitive triggering, curve drawing tools, metering calibration and automation stress testing.

## Available validation

- The JUCE-independent production timing helper and its regression assertions were **compiled and executed locally with g++ C++17, -O2 -Wall -Wextra -Werror** on Linux (passed). This confirms only pure timing maths, not VST3 compatibility.
- The new JUCE integration target, Windows Release, pluginval, GUI, Ableton crash safety and all on-audio tests **have not been executed** since the latest modifications.
- The plugin includes 33 APVTS parameters; all are referenced in current editor code, although this does not establish auditory correctness or feature completeness.

## Release gate
Workflow must remain `workflow_dispatch` only. PR #17 must not merge to `main` without explicit approval. The source is under active development; a passing CTest is not proof of perceptual sound quality or crash-free Ableton operation.
