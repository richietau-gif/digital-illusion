# PRECISION SIDECHAIN 1.0 implementation audit

Updated 2026-11-10.

Implemented in the current branch:

- Single-envelope processor and GUI with no sequencer or modulation grid.
- Audio, internal audio, external sidechain, MIDI, and combined triggers.
- Host BPM-derived envelope lengths, bounded lookahead delay, reported latency, aligned bypass, attack protection, smoothing, depth, mix, and output gain.
- Filter coefficient construction moved out of the per-sample loop; selectable filter branches are connected to processing.
- Six connected color algorithms.
- Persistent UI scale parameter and resizable editor.
- 32 factory programs with complete parameter variation.
- Manual Windows workflow builds all targets, runs CTest, runs pluginval strictness 5, and packages the VST3.
- Standalone timing regression test passes locally with strict warnings.

Still requiring Windows verification:

- JUCE/CMake compilation, CTest under Windows, pluginval, VST3 output, and Ableton validation.

Known functional limitations:

- The envelope graph is currently visual-only; node insertion, curvature editing, undo/redo, and curve state serialization are not implemented.
- A/B snapshot controls and user preset file browser are not implemented.
- Creative filter branches are connected approximations rather than dedicated analog ladder, comb, or formant algorithms.

## GUI/DSP refinement (2026-11-10; pending Windows validation)

- Redesigned the editor around a logical 1100x700 canvas, with a 75% default viewport of 825x525 and selectable 50%/75%/100% sizes. The saved `uiScale` parameter is retained.
- Added legible knob/choice labels, compact numeric formatting with % and meaningful units, custom clean rotary arcs, responsive controls, and discrete dark panels.
- The central orange gain envelope preview now reflects current depth, timing-offset and attack-protection parameter values. The cyan graph element is intentionally labeled as a **cutoff marker**, not a falsely precise DSP response curve.
- Added a bound global bypass button, trigger activity/output metering and in-editor navigation for 32 factory programs.
- Aligned the Dry/Wet mixture to the same delayed signal, corrected zero-delay handling, and ensured bypass skips filter/color/output processing.
- Checked source-level connections and confirmed Windows CI remains manual-only. **No Windows build, pluginval or Ableton QA has yet been run for these new commits.**

Outstanding work: interactive envelope node editing, comprehensive genuine filter algorithms, precise negative-offset DSP/host latency verification, user preset files, A/B snapshots, and actual GUI screenshots/QA at 50%/75%/100%.

## Reference design implementation (2026-11-10; requires a new Windows build)

- Implemented an original JUCE vector GUI following the approved PRECISION SIDECHAIN mockup, with a 75% (825x555) default, 50%/75%/100% scale selector, navy panels, orange editable envelope, cyan filter-response preview, lime filter/color switches and responsive layout.
- The envelope offers left-click add, left-drag reposition, right-click delete and Shift-drag curvature. Up to 12 nodes drive real audio envelope interpolation; node coordinates/tension are included in project state. Editor Undo/Redo buttons are implemented, but undo history is editor-local.
- The header has factory-program navigation, A/B comparison and copy A->B; A/B snapshots are currently editor-session-local and require host-persistent snapshot QA.
- Added separate automatable parameters: `filterEnabled`, `filterEnvelopeEnabled` (modulation gate), `colorEnabled`, `colorTone`, `detectorSource`, and `curveAmount`. FILTER ENV OFF preserves the filter but removes envelope modulation of cutoff.
- Added LP/HP 12/24/48 cascades, a true bandpass/notch DSP path and a bounded comb-feedback filter. Other creative models remain approximations and need further fidelity testing.
- Added an automatable CURVE tension parameter and header slider; improved 32 programs with six distinct single-cycle envelope types. Filter drive now affects standard filter models. The filter envelope still needs on-host automation and DSP tests.
- Added spectrally meaningful COLOR tone control; the SAVE button exports the plugin state to a file. Import/browser support is still outstanding.
- Updated presets to provide several distinct envelope shapes (precision duck, gate, one-cycle wobble, reverse, double pump, S-rise).
- Filter display is an approximate frequency response preview, **not** a measured response curve; visualizer should be upgraded to match each exact filter algorithm.
- Updated source-level sanity checks found and fixed an invalid `elseif` token in the filter branch. No JUCE compilation, CTest, pluginval or Ableton screenshot/host validation has run since this larger refactor; **do not treat this branch as a validated RC**.

## Remaining critical work

- First Windows build on the new commits and pluginval strictness-5 validation, then inspect 50/75/100% JUCE screenshots and test real Ableton interaction.
- Real-time and host PDC validation of live lookahead changes and ±100% negative offset behavior. The current time offset must not be represented as fully sample-accurate negative lookahead without further tests.
- Robust host-persistent A/B snapshots, complete user-preset import/browser and full parameter-automation testing.
- Improve special filter algorithms (ladder, formant, morph) and oversampling/anti-aliasing; add actual DSP integration tests for the filter-envelope toggle and cutoff-rate automation.
