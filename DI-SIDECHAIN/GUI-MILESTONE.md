# DI SIDECHAIN — GUI milestone

This branch introduces the first native-JUCE implementation of the approved
charcoal/coral DI SIDECHAIN layout. The current processor and VST3 identity
are intentionally preserved.

## Controls implemented

- **Top selectors:** Trigger (Audio+MIDI, Audio, MIDI), **MODE (One Shot / Loop)**,
  and rhythmic division. LENGTH offers 1/32 through 2 bars, plus appended
  1/16T, 1/8T, 1/4T triplets and 1/16D, 1/8D, 1/4D dotted choices.
  Original division indices 0–6 have not changed, preserving old Sets.
  FULL/LOW/HIGH/BAND remain in Frequency Target below.
  **One Shot** starts an envelope at every accepted audio or MIDI event.
  **Loop** repeats automatically. When the host provides transport PPQ and tempo
  with SYNC enabled, the loop is locked to the musical timeline; otherwise
  it falls back to its internal phase oscillator. Stopping the host silences
  looping ducking when a host playing flag is available.
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
- **Frequency Target:** live 24-bar log-frequency spectrum derived from a
  2048-point FFT of the stereo input **before** gain reduction, refreshed
  on the GUI message thread. Choose FULL, LOW, HIGH or BAND. Drag a
  boundary to change crossover frequency; the selected region updates live.
  The spectrum shows INPUT energy, not precise frequency-dependent
  gain reduction. Bars decay when no new input arrives.
- **Bottom:** active DSP Depth, Smooth, and Mix parameter sliders.
- **Advanced Controls:** expand to reveal Threshold, Length in milliseconds,
  cutoff frequencies, Retrigger/Holdoff, MIDI note and MIDI channel, plus
  **TRIGGER HP Hz** (0 = off, up to 2000 Hz). This applies a detector-only
  high-pass to the external audio sidechain and does not filter the audible
  program signal. Default 0 preserves the behavior of older projects.
  MIDI note -1 = all notes, 0–127 = specific note number; MIDI channel 0 =
  all channels, 1–16 = one channel. These parameters are automatable and saved
  with the host project. Incoming MIDI note-on triggers use host sample offsets
  and can retrigger at irregular breakbeat intervals.

## Creative OUTPUT FILTER for wobble sound design

- **FILTER** checkbox beside Frequency Target toggles this entire output
  stage; default OFF preserves existing Ableton Sets and build #64 behavior.
- **LP / HP / BP / Notch** selectable filter shapes (indices 0/1 retain prior
  Low Pass / High Pass settings). A topology-preserving SVF filter has smooth
  per-sample cutoff modulation suitable for wobble curves.
- Select **12 dB** (one state-variable stage) or **24 dB** (two stages).
  **RESONANCE Q** from 0.5 to 12 emphasizes the cutoff (high Q can be loud).
  **FILTER DRIVE** 0–100% adds soft saturation before the filter, with 0%
  remaining transparent. Drive uses a first-order ADAA tanh shaper to reduce
  aliasing compared with naive per-sample tanh. This is not equivalent to
  oversampling; some aliasing at extreme settings can remain, and the high-Q
  / full-drive combinations require practical Ableton validation.
- **FILTER MIN Hz** and **FILTER MAX Hz** set the cutoff boundaries.
  **FILTER AMOUNT** sets how strongly the VOLUME ENVELOPE drives cutoff.
  At 100%, normalized envelope 0 = MIN cutoff and 1 = MAX cutoff;
  at 0%, cutoff stays at MAX. Volume **DEPTH** may remain at zero, allowing
  purely rhythmic filter motion without volume pumping.
- The creative output filter operates on program audio *after* gain shaping.
  It is independent of **TRIGGER HP**, which only filters the external audio
  detector. Enabling/disabling FILTER crossfades over 5 ms.
- Parameters are automatable, serialized in host state, and have regression
  tests for spectral modes, drive, resonance, slope, bypass and roundtrip.

## MULTI – independent three-band ducking

- **MULTI** is now selectable beside FULL / LOW / HIGH / BAND.
- Two crossover frequencies LOW and HIGH split the audio into LOW / MID /
  HIGH via complementary Linkwitz-Riley paths with phase-alignment.
- **MULTI LOW DEPTH**, **MULTI MID DEPTH**, **MULTI HIGH DEPTH** are separate
  host-automatable band ducking scales (0 = retain band, 1 = full ducking).
  Main DEPTH remains the master envelope modulation scale.
- The extra band sliders appear under Advanced Controls for this milestone.
  Tests cover selective band gain, depth-zero transparency and persistence.
- A later dedicated 3-band panel and additional filter quality modes are
  planned; this version has no independent envelopes per band.

## External audio detector dynamics

- **TRIGGER ATTACK ms** (0–30 ms; 0 = immediate peak response) and
  **TRIGGER RELEASE ms** (1–250 ms; default ~4.16 ms matches legacy decay
  near 48 kHz) control the trigger envelope without changing the audible
  output directly. They work with existing threshold, holdoff and TRIGGER HP.
- Added tests for short transient rejection, fast retriggering, slow release
  suppression and saved host state.

## Not implemented yet

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

## CI verification for this revision

Run the Windows VST3 workflow against the **feature/di-sidechain-v1-gui**
branch or let a pull-request synchronization commit trigger it. The run must
compile the VST3, execute CTest, run pluginval and upload the VST3 artifact.
A failed or cancelled run is not a verified plugin release.
