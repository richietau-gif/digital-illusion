#pragma once
namespace di {
// MIDI note-on is intentionally immediate; audio peaks obey the holdoff.
// Combining both sources produces only one envelope restart per sample.
// A detected crossing is consumed even when holdoff suppresses the trigger.
// The detector rearms only after falling below its lower hysteresis threshold.
inline bool shouldConsumeAudioPeak(bool audioPeak,bool audioArmed,int mode) noexcept {
 return mode!=2&&audioPeak&&audioArmed;
}
inline bool shouldFireTrigger(bool midiNoteOn,bool audioPeak,bool audioArmed,
                              int holdoffRemaining,int mode) noexcept {
 const bool midiAllowed=mode!=1;
 const bool audioAllowed=mode!=2;
 return (midiAllowed&&midiNoteOn) ||
        (audioAllowed&&audioPeak&&audioArmed&&holdoffRemaining<=0);
}
}
