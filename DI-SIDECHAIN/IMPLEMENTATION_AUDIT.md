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
