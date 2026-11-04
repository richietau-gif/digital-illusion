# DI SIDECHAIN — GUI milestone

This branch introduces the first native-JUCE implementation of the approved
charcoal/coral DI SIDECHAIN layout. The current processor and VST3 identity
are intentionally preserved.

## Controls implemented

- **Top selectors:** Trigger (Audio+MIDI, Audio, MIDI), Frequency Mode
  (Full, Low, High, Band), rhythmic division.
- **SYNC:** toggles tempo-synced envelope length. Switch it off to use the
  milliseconds LENGTH parameter in Advanced Controls.
- **Volume envelope:** between 2 and 32 freely positioned DSP-backed nodes.
  Drag a node horizontally (time) or vertically (gain). Double-click empty
  plot space to insert one, right-click an interior node or press Delete to
  remove it. First and last nodes are fixed at normalized time 0 and 1.
  **SNAP: OFF/ON** quantizes time and gain to 1/16 steps. Left/right arrows
  choose nodes, Shift+left/right moves their time, up/down adjusts gain.
  Existing presets with eight evenly spaced nodes remain loadable.
  **Alt+drag** a graph segment vertically to shape its cubic Bezier easing;
  each segment has an independent curvature and saves with the Ableton Set.
  The plotted curve uses the exact same DSP evaluator.
- **Frequency Target:** choose FULL, LOW, HIGH or BAND. In LOW/BAND, drag a
  handle to change crossover frequency; the selected region updates live.
- **Bottom:** active DSP Depth, Smooth, and Mix parameter sliders.
- **Advanced Controls:** expand to reveal Threshold, Length in milliseconds,
  cutoff frequencies, and Retrigger/Holdoff.

## Not implemented yet

- The bars in Frequency Target are an **illustration**, not a live FFT.
- **MULTI** mode is visibly disabled; no multiband DSP yet.
- Curves now support per-segment cubic shaping via Alt-drag, but do not
  yet expose independent two-handle bezier controls or segment presets.
- The GUI curve and DSP now use the same evaluator; no merely decorative
  Bézier approximation is used.
- Ableton Live 12 Suite integration has **not** been verified automatically.
  Follow `Tests/AbletonSmokeTest.md` in a disposable session.

## Release safeguards

Merge the stability PR #1 before merging this GUI PR. The GUI PR uses the
stability branch as its base. GitHub Windows CI must pass both CTest and
pluginval before any human install test; a green CI result alone does not
prove host compatibility.
