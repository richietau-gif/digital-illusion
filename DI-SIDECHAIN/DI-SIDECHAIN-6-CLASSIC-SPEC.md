# DI SIDECHAIN 6.0 CLASSIC — single-envelope, filter-first redesign

Status: implementation specification. The branch is **not** yet a verified build.
Base: di-sidechain-5.1-dev. Keep versions 5.0 and 5.1 intact. Do not merge into main until expressly approved.

## Product direction

Build the most refined SINGLE-ENVELOPE sidechain effect possible, with meaningful sound-design FX. **No step sequencer**, no step thumbnails, no modulation lanes disguised as sequencers. Emphasize a large, directly editable envelope, a sophisticated filter, accurate audio/MIDI triggering, time alignment, and quick exploration with complete factory/user presets. Preserve host automation IDs for existing parameters and safe loading of old project state; ignore/hide obsolete sequence features without silently deleting saved data.

### Core signal path and design
1. Main audio L/R, optional sidechain detector input, MIDI notes. Maintain audio/MIDI trigger modes; avoid real-time allocations, blocking calls and file I/O.
2. Reliable trigger detector with sensitivity/threshold, hysteresis, holdoff, optional retrigger and envelope duration sync to Ableton BPM/time signature, deterministic behavior on transport jumps.
3. One multi-point envelope. Left-click adds a point, right-click removes, drag moves, Shift+drag changes curvature/tension. Undo/Redo, snapping and shape presets. Migrate legacy eight-node envelopes to the new format without destroying old project compatibility.
4. Two independent depth controls: **volume duck** and **filter envelope depth**, optionally with a clean low-frequency-protection mode.
5. **Timing offset** [-100%, +100%] of active envelope duration, with center at 0%; draw both trigger marker and shifted response. Clarify units in synced (beats) and free (ms) modes. IMPORTANT: negative/anticipatory offset for unpredictable audio/MIDI triggers is non-causal. Either use a fixed maximum lookahead audio delay with correct host-reported latency, precomputed song-position triggers where available, or explicitly constrain pre-trigger shifting with clear UX. Do not claim genuine lookahead without delaying audio and validating latency/PDC.
6. Filter models: clean 12/24 dB LP/HP/BP/notch, resonant analog-style ladder, SVF with resonance and morph (LP–BP–HP), comb/phaser-like color, formant/vowel-style filter and optionally a steep 48 dB mode. Model names must match real DSP. Parameter smoothing, no unstable coefficients at all supported sample rates. Resonance / cutoff / drive / envelope amount / mix controls, responsive response graph.
7. Optional distortion/color, not mandatory on every preset: independent saturator, soft/hard clipper, configurable oversampling quality only if implemented and tested. Do not add delay/reverb merely as UI decoration.
8. Input/output meters, gain reduction envelope and filter-response visualization. Preserve dry/wet/bypass alignment and report actual PDC. Prevent zipper noise/clicks.
9. Presets: broad factory categories DUCKING, DUBSTEP / FUTURE RIDDIM, BASS MOVEMENT, TRANSITIONS, SUB CLEANUP, MIXING, FILTER FX. Aim for 24+ meaningful presets, with original sound-design settings evoking modern cinematic bass, growls, vowel movement, gated swells, reese textures, rhythmical wobble modulation **within a single envelope**. An envelope-based effect cannot autonomously generate sustained multi-step wobble patterns without a periodic repeat/LFO; if implemented, provide a SIMPLE retriggerable envelope repeat / LFO switch, never resurrect a step sequencer. Distinguish presets with audible parameter and curve differences. Names and content should be original, not copies of a particular artist's work.
10. Preset browsing by category; searchable if practical; user save/load, A/B snapshots and A→B / B→A, compatibility and size limits. Clearly signal modified preset.

## GUI (premium, pragmatic)
Dark navy/near-black vector UI, orange central envelope, cyan filter traces, lime active switches; premium typography and spacing. No ornamental ellipses on knobs. A large central editable envelope, a full-width filter response graph, consistent rotary controls, wide sliders, uncluttered header with preset browser/A-B/setup. Setup stays hidden unless clicked; GUI scaling 50/75/100% with 75% default for new instances; keep saved project scale. Provide keyboard navigation, tooltips, practical numeric units, legible at 50%.
Layout: HEADER, ENVELOPE (largest), DETECTOR/TRIGGER and TIMING, FILTER (always visible, visual response), COLOR/OUTPUT. No sequencer pane or step controls.

## Acceptance gates (all required before RC label)
- JUCE Windows VST3 Release compiled by the Windows workflow on this branch; CTest pass, pluginval strictness 5 pass, VST3 ZIP artifact verified.
- Real-time-safe envelope updates and automation; tests for point add/delete/tension/shape migration, timing offset incl. 0/±100%, audio and MIDI triggering, BPM and transport loops, fixed PDC across bypass configurations, sample rates 44.1/48/96k and common buffers.
- Filter responses, stability at high resonance, NaN/Inf automation protection, saturation aliasing, phase and low-frequency preservation checks.
- GUI lifecycle, scaling/hit-testing 50/75/100%, Setup popup, preset A/B and state migration, screenshots at each UI scale and Ableton Live 12 interactive test.
- Don't trigger GitHub Actions without explicit user permission. Keep workflow_dispatch only and never merge into main by default.

## Implementation order
1. Remove step-sequencer UI and rebuild a compact single-envelope layout while preserving legacy state.
2. Introduce a DSP-safe dynamic multi-node envelope and interpolation tests.
3. Implement correct ±100% timing offset and fixed lookahead/PDC tests.
4. Expand filter architecture and tests.
5. Distinctive single-envelope repeat/LFO option and curated future-riddim/dubstep presets.
6. Polish GUI and state migration, then request one explicit Windows build approval.

### Notes on sonic references
Dr. Ushuu's modern future-riddim/bass music is a *direction* for original presets and varied sound design; build new sounds rather than duplicating any recordings, protected patches, or artist branding. Focus on formant movement, warm-to-aggressive filtered bass, distortion harmonics, clean sub management and punchy ducking.
