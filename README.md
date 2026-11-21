# Siren

VST3 / AU drone instrument after the Eurorack module *Siren*. Play one note (or press START DRONE): the note is the root,
and 21 oscillators stacked in Just Intonation swell in over a very long attack. No CV inputs; everything is a knob.

**Drone**: Root (when no MIDI is playing), Attack / Release (up to 120 s / 180 s), Stagger (root triad enters first, upper layers follow), Glide.
MIDI note = root, sustain pedal and pitch bend supported.

**Timbre**: Chord morphs smoothly through 10 Just Intonation sets (octaves, power, minor triad, inversions, open, min7, min9, sus2, dorian cluster).
Spread = octave spread, Timbre = wavetable position, Index = wavetable, Animate = auto detune, stereo width and LFO speed.

**Bundles A/B/C**: 7 oscillators each (low / mid / high) with their own filter (LP / BP / HP, cutoff, resonance, level) and a manual detune (+-50 ct) per oscillator.

**LFO matrix**: 8 LFOs (rate, shape) x 24 targets (pitch of each oscillator, cutoff of each filter). Drag a cell for depth, double-click for 0.
Pitch LFO / Filter LFO set the maximum depth. Modulation grows with the swell.

**Space**: shimmer reverb (octave-up feed-forward and feedback), then a look-ahead brickwall limiter (3 ms, adjustable ceiling).

Build: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` (JUCE 8.0.6 is downloaded automatically, or pass `-DJUCE_PATH=/path/to/JUCE`)
Offline engine test: `g++ -O2 -std=c++17 tests/render_test.cpp -o tests/render_test && ./tests/render_test`
