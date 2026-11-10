# PRECISION SIDECHAIN — AUTHORITATIVE GUI REFERENCE CONTRACT
Status: implementation contract; **source changes are NOT yet Windows-compiled or pluginval/Ableton-validated**.
Reference: approved user-supplied 1536 × 1024 mockup in the current project conversation (10 Nov 2026). Do not replace this reference with generic FabFilter-like ideas.

## Visual acceptance

- Preserve the approved dark graphite/navy aesthetic, fine outlines, restrained gradients/shadows, cyan audio controls, orange envelope and lime-green active power switches.
- Match relative placement and density: header (0–~9%), single envelope editor (~9–46%), trigger/timing/envelope/mix (~48–73%), filter/color (~74–98%). No sequencer panel or modulation-grid UI.
- Native JUCE logical canvas: 1100 × 740; initial 75% window **825 × 555**. Scale choices 50% (550 × 370), 75% (825 × 555), 100% (1100 × 740), persisted through APVTS.
- The source is vector JUCE rendering, not an image baked into a background. Check actual fonts, strokes and hitboxes on Windows at all 3 sizes.
- Selected knobs must have smooth cyan arcs; Timing Offset is orange; no oval decorations.
- Avoid overlapping popups, cropped value boxes, unreadable labels, or partially hidden controls. Target minimum legible value labels at 75%.
- Do not claim pixel-perfect match until comparing a real screenshot of compiled VST3 with the attached mockup.

## Actual controls and parameter mapping

| Reference element | JUCE control / DSP state | Acceptance |
|---|---|---|
| Header title & logo | vector rendering | PRECISION SIDECHAIN / DIGITAL ILLUSION, no obsolete DI SIDECHAIN in user-facing UI |
| Factory preset browser, prev/next | `presetBrowser`, `prevPreset`, `nextPreset` -> 32 complete factory programs | Selecting programs changes DSP and curve; browser follows host program |
| Save | `savePreset` -> binary plugin state export | Export works, async file picker; load/import remains to be done |
| A/B + Copy A->B | `aButton`, `bButton`, `copyAB` | Editor-session snapshots currently, host-persistence still needed |
| Undo/Redo | `undoButton`, `redoButton` | Curve edits; command-key shortcuts to be added/verified |
| Setup + Power + Bypass | `settingsButton`, `powerButton`, `bypass` | Setup changes UI scale; Power is inverse of Bypass |
| EDIT / BEND / ERASE / RESET | `selectTool`, `bendTool`, `eraseTool`, `clearCurve` | Click add, drag move, Shift+drag or Bend adjusts tension, right-click/Erase removes internal node |
| GRID dropdown | `gridPicker` -> `editGrid` | Actual snap Off/1/4/1/8/1/16, Alt temporarily disables snap. **Not the BPM Rate** |
| CURVE knob/slider | `curveAmountSlider` -> `curveAmount` | Actual global interpolation/tension in envelope DSP |
| Graph | shared `getEnvelopeAt` / `setEnvelopeNodes` / `envelopeRevision` | Orange output curve reflects envelope, depth, timing offset and attack protection; data serialized |
| ENV/OUT meters | `gainReductionMeter` and `outputMeterL/R` | Actual DSP levels; not decorative animations |
| Trigger dropdown + INT/EXT/MIDI | `triggerMode`, `detectorSource` and selector buttons | Strict external trigger never silently falls back to internal; detector modes Peak/RMS/Transient functional |
| Timing Length | Free `length` slider, Sync `division` slider | Same physical position; synced rate uses musical divisions; BPM SYNC is independent |
| Time Offset | `timeOffset` | ±100% control; **true negative anticipation and PDC need full regression testing** |
| Lookahead | `lookahead` | Actual delay tap and asynchronously reported host latency, with crossfade on changes |
| Envelope Attack / Release / Depth | `attackMs`, `releaseMs`, `depth` | Separate gain smoothing directions, real DSP |
| Attack Protection | `attackProtection` | Toggle bound to existing 0–1 attenuation protection parameter |
| Mix Dry/Wet, Output | `mix`, `output` | Time-aligned dry/wet and output gain |
| FILTER ON | `filterEnabled` | Full filter wet branch bypassed, no unintended coloration |
| Filter model | `filterModel` | Multiple models; LP/HP 12/24/48, BP, Notch, Comb are distinguished; several creative models approximate |
| Filter Cutoff/Q/Drive | `cutoff`, `resonance`, `filterDrive` | Real filter processing, no inert knobs |
| **FILTER ENV ON/OFF** | `filterEnvelopeEnabled` | ON modulates cutoff by single envelope; OFF preserves filter but disconnects envelope |
| Filter ENV depth / Filter Mix | `filterMod`, `filterMix` | Additional compact controls beneath response graph |
| Filter response graph | analytical *approximation* in GUI | Must not be described as exact transfer function; upgrade for nonlinear/morph models |
| COLOR ON / Type / Drive / Tone / Mix | `colorEnabled`, `colorModel`, `colorDrive`, `colorTone`, `colorMix` | Real processing, tone is split-based tilt rather than an inert control |

## Defects / risk gates not yet closed

1. **No Windows build for the latest reference-GUI changes.** Actual JUCE drawing, font fallbacks, compilation, pluginval editor lifecycle, resize, close/reopen and DAW automation must be tested.
2. A/B state currently lives in the editor; persist with host project, ensure copying full envelope and parameters and robust serialization.
3. Factory and user preset import/export and preset state editing need deeper testing; only file export is currently implemented.
4. Filter response display is approximate, particularly driven, comb, formant and analog-style modes; do not imply exact scientific visualization.
5. True negative pre-trigger offset for unpredictable MIDI/audio triggers is physically impossible without adequate lookahead; verify configured bounds, plugin-delay compensation, bypass and automation. Latency changes can be host-dependent.
6. Confirm stable filter DSP for 44.1/48/96/192 kHz, high resonance, click-free mode changes, denormals and oversampling.
7. Confirm GUI node hit-testing at 50/75/100%, undo/redo, grid snapping and right-click actions.
8. Test all 33 registered parameters and all GUI controls against actual audio-processing behaviour. Keep Windows workflow manual-only. Never merge PR #17 without explicit approval.

## Release criteria

Only mark a release candidate verified when a user-approved Windows workflow compiles all targets, CTest passes, pluginval strictness 5 passes, VST3 ZIP is uploaded and tested in Ableton Live 12. For visual approval, compare a real VST3 screenshot at 75% side by side with the authoritative user render.
