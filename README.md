# Siren

VST3 / AU drone generator after the Eurorack module *Siren*: 21 oscillators stacked in Just Intonation, 23 LFOs, macro controls X / Y / Z.
Oscillators are intentionally not anti-aliased.

| Control | Function |
|---|---|
| X | Chord selection, smoothly morphing between 10 Just Intonation stacks (far left = octaves) |
| Y | Octave spread of the oscillators (inversions); LFO1 normalled in via the Y attenuverter |
| Z | Wavetable position; LFO2 normalled in via the Z attenuverter |
| Pitch | Base pitch, 12 o'clock = C. Lock prevents accidental changes |
| Index | Selects wavetable (also LFO waveforms). About 3 o'clock = pseudo-VCA table (Z = volume) |
| Animate | Detune, stereo width, LFO speed |
| Nearest | While on, X/Y/Z snap instead of interpolating |
| Reset | Resets oscillator and LFO phases (momentary) |
| V/Oct 2, LFO speed, PMod, Idx<-LFO1 | Extra modulation inputs. All knobs are automatable, so they work as CV from the host |

MIDI: note = V/Oct (C3 = 0 V), pitch bend, mod wheel = X CV. PMod can use the optional sidechain input as audio-rate phase modulation.

Build: `git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git libs/JUCE && cmake -B build && cmake --build build`
