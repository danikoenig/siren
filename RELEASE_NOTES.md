### New in v1.3.0
- **Reset and Panic**: right-click the header (or click SIREN) for "Reset all to default" and "Panic: silence now" (stops the drone and the reverb tail at once)

### New in v1.2.4
- **Reverb rebuilt**: 16-line network with modulated delays and stronger diffusion, so it no longer rings on held drone notes
- Fixed a glitch in the Tone filter and level jumps when turning Decay

### New in v1.2.3
- **Manuals**: new PDF manuals in English and German, attached to this release (also linked on the download page)
- Manual covers quick start, every control, recipes and install

## Siren v1.2.2

- **CLAP format** (Bitwig, Reaper ...) in addition to VST3 and Audio Unit. The installer puts it in /Library/Audio/Plug-Ins/CLAP.

Drone generator after the Eurorack module Siren (VST3 / Audio Unit, Apple Silicon).

- Play one note: 21 oscillators in Just Intonation fade in over a very long attack and fade out with the release
- Chord morph through minor triads, inversions, min7/min9, sus2 and a dorian cluster
- 3 oscillator bundles (low / mid / high) with their own filter and a fine detune (supersaw)
- One LFO per bundle, routed with an XY pad (cutoff / reso / level / pitch)
- Reverb with long decay, pre-delay and filter-style tone, plus a limiter at the end

**Install:** download `Siren-Installer.pkg` and follow `INSTALL.txt` (Apple Silicon, macOS 11+).
- Stereo: Width (widening) and Move (slow L/R drift)
- Out: Limiter threshold with auto gain (like Drift Station) and Level
- New init sound
- Window opens larger (1180 px wide, same width as Quad Shooter)
