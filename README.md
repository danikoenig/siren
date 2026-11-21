# Siren

**Manual (PDF):** [English](https://github.com/danikoenig/siren/releases/latest/download/Siren-Manual-EN.pdf) | [Deutsch](https://github.com/danikoenig/siren/releases/latest/download/Siren-Manual-DE.pdf)

VST3 / AU / CLAP drone instrument after the Eurorack module *Siren*. Play one note (or press START DRONE): the note is the root,
and 21 oscillators stacked in Just Intonation swell in over a very long attack. No CV inputs; everything is a knob.

**Drone**: Root (when no MIDI is playing), Attack / Release (up to 120 s / 180 s), Stagger (root triad enters first, upper layers follow), Glide.
MIDI note = root, sustain pedal and pitch bend supported.

**Timbre**: Chord morphs smoothly through 10 Just Intonation sets (octaves, power, minor triad, inversions, open, min7, min9, sus2, dorian cluster).
Spread = octave spread, Timbre = wavetable position, Index = wavetable, Animate = auto detune, stereo width and LFO speed.

**Bundles A/B/C**: 7 oscillators each (low / mid / high) with their own filter (LP / BP / HP, cutoff, resonance, level) and a fine detune (supersaw). FM: C modulates B, B modulates A.

**LFO per bundle**: rate, shape, depth. An XY pad routes it (corners: cutoff, reso, level, pitch). Modulation grows with the swell.

**Cross modulation**: FM from bundle C into B and from B into A (phase modulation, one slider each).

**Space**: reverb (decay up to 60 s, tone as LP/HP), stereo widening with a slow L/R drift, then a look-ahead limiter with threshold and auto gain (ceiling -1 dBFS).

Build: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` (JUCE 8.0.6 is downloaded automatically, or pass `-DJUCE_PATH=/path/to/JUCE`)
Offline engine test: `g++ -O2 -std=c++17 tests/render_test.cpp -o tests/render_test && ./tests/render_test`
