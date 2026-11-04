# DI SIDECHAIN — GUI milestone

This branch introduces the first native-JUCE implementation of the approved
charcoal/coral DI SIDECHAIN layout. The current processor and VST3 identity
are intentionally preserved.

## Controls implemented

- **Top selectors:** Trigger (Audio+MIDI, Audio, MIDI), Frequency Mode
  (Full, Low, High, Band), rhythmic division.
- **SYNC:** toggles tempo-synced envelope length. Switch it off to use the
  milliseconds LENGTH parameter in Advanced Controls.
- **Volume envelope:** eight DSP-backed control points. Click/drag a point
  along the vertical axis (points are evenly spaced in time). Click
  **SNAP: OFF/ON** to quantize gain to 1/16 steps. Double-click a node to
  restore 100% gain. After selecting a node, left/right arrows select adjacent
  points and up/down arrows adjust gain.
- **Frequency Target:** choose FULL, LOW, HIGH or BAND. In LOW/BAND, drag a
  handle to change crossover frequency; the selected region updates live.
- **Bottom:** active DSP Depth, Smooth, and Mix parameter sliders.
- **Advanced Controls:** expand to reveal Threshold, Length in milliseconds,
  cutoff frequencies, and Retrigger/Holdoff.

## Not implemented yet

- The bars in Frequency Target are an **illustration**, not a live FFT.
- **MULTI** mode is visibly disabled; no multiband DSP yet.
- Eight fixed, evenly spaced nodes are retained for compatibility with the
  current processor. Arbitrary node insertion and Bézier tension handles are
  future work in Issue #5.
- UI envelopes use a visually smoothed path; DSP still uses the existing
  interpolation. Matching the displayed curve to exact DSP gain is future work.
- Ableton Live 12 Suite integration has **not** been verified automatically.
  Follow `Tests/AbletonSmokeTest.md` in a disposable session.

## Release safeguards

Merge the stability PR #1 before merging this GUI PR. The GUI PR uses the
stability branch as its base. GitHub Windows CI must pass both CTest and
pluginval before any human install test; a green CI result alone does not
prove host compatibility.
