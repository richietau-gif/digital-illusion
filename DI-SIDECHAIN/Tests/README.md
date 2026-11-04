# Regression validation

Configure with `-DBUILD_TESTING=ON`, build, then run
`ctest --test-dir build -C Release --output-on-failure`.
The executable needs a desktop/message loop (use Xvfb on headless Linux).
Checks use runtime failures in Release builds, rather than disabled assertions.

The editor test constructs before preparation, after preparation, and after
state restoration; checks every control has initial bounds; paints, resizes,
automates parameters from another thread, and repeatedly destroys/reopens the
editor while dispatching pending callbacks. Moving setSize before control
construction must fail this test (the original crash).

Processing checks cover disabled/mono/stereo external buses, all frequency modes,
44.1/48/96 kHz, MIDI ducking and consumption, and parameter/envelope persistence.
Windows CI additionally runs pluginval at strictness 5 against the built VST3.

## Investigation evidence

Run 41693602708 (commit 6d062b6) stopped at pluginval's Editor test.
The editor called setSize before constructing controls; JUCE calls resized
synchronously, so its control accesses could crash during loading in a host.
Commit 8bc501f already moved setSize after construction and guarded resized.
Preserve that ordering.

Run 41699422380 on that fixed commit completed Editor, Open editor whilst
processing, Editor Automation, audio, state and bus tests and printed SUCCESS.
The workflow nevertheless failed: PowerShell tested a null/stale LASTEXITCODE
while pluginval, a Windows GUI executable, was still running. Start-Process,
an explicit wait/watchdog and the process's ExitCode make validation reliable
without accepting failures or suppressing editor tests.

Ableton Live 12 Suite must also be checked manually on Windows: install the
new artifact, rescan, load a fresh instance, reopen the editor, restore a saved
set, and test external mono/stereo sidechain plus MIDI triggering. CI/pluginval
does not establish that this proprietary host has been tested.
