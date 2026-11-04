# Ableton Live 12 Suite — DI SIDECHAIN smoke test

**Use a blank Set only.** Save/close important sessions before installing an experimental VST3.

## Before testing
1. Exit Ableton Live.
2. Move any older `DI SIDECHAIN.vst3` bundle out of `C:\Program Files\Common Files\VST3\` so only the current build remains.
3. Extract the Windows GitHub Actions artifact and install the whole `DI SIDECHAIN.vst3` **directory** in `C:\Program Files\Common Files\VST3\`.
4. Open Ableton Live 12 Suite and rescan plug-ins in Preferences. Do not open an important project.

## Manual pass/fail checklist
- [ ] **Insert:** Add DI SIDECHAIN to a fresh stereo audio track. No crash or frozen Live UI.
- [ ] **GUI:** Open/close the editor 10 times; resize where supported; interact with all sliders and combo boxes. No crash, glitch, or invalid values.
- [ ] **Transport:** Start/stop playback, change tempo (90, 120, 174 BPM), then stop. No crash or clicks beyond the expected processing.
- [ ] **Disconnected sidechain:** Process audio without any external sidechain routing. Output remains finite and consistent.
- [ ] **Audio trigger:** Enable the external sidechain input in Ableton and route a kick/transient. Select Audio only. Verify the output ducks on kick events at several thresholds. Check mono and stereo sidechain sources if the host makes those available.
- [ ] **MIDI trigger:** Send MIDI note-on events to the audio effect using supported Ableton routing. Select MIDI only, confirm retriggering and output gain reduction, including irregular breakbeat notes. If routing is not available, mark *not verified*, not *pass*.
- [ ] **Frequency modes:** Test FULL, LOW, HIGH and BAND with low- and high-frequency audio; listen for clicks, excessive gain or phase artifacts.
- [ ] **Automation:** Automate Depth, Mix, Smooth and cutoff parameters during playback. No crash or invalid DSP output.
- [ ] **Save and reopen:** Change envelope points and parameters, save a Set, close Live, reopen the same Set. Confirm controls and curve restore.
- [ ] **Sample rate/buffer:** Repeat insert, audio and transport checks at 44.1/48/96 kHz and 64/256/512 samples *where supported by the audio interface*.
- [ ] **Remove:** Remove the plug-in during stopped transport and during playback, then undo if supported. No crash.

## Recording results
For any failure, record Ableton exact version/build, Windows version, sample rate,
buffer size, plugin artifact build URL/SHA, host track type, whether GUI was
open, triggering mode, and exact reproduction steps. Attach Live crash log
or crash report if available. Do not use the failing build in production Sets.

## Important
A successful Windows CTest/pluginval run proves only those automated checks
passed. It **does not** substitute for testing inside Ableton Live.
