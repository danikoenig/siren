try:
    from lib import install_text
except ImportError:
    pass

CONTENT = {
"title": "SIREN", "file": "Siren", "repo": "siren",
"accent": "#ffab4a", "accent2": "#ff7a2f",
"en": {
 "subtitle": "Drone generator",
 "kind": "Instrument for macOS (Apple Silicon)  \xb7  VST3, Audio Unit, CLAP and Standalone",
 "by": "by Dani Koenig",
 "intro": ("Siren is a drone generator. Play one note and 21 oscillators swell in slowly, from a few seconds to minutes, and fade out again when you let go. "
           "They sit in three bundles (low, mid, high), each with its own filter, fine detune and LFO. "
           "Chord sets in Just Intonation, a long reverb and a limiter at the end turn one key into a huge, slowly moving sound."),
 "note": "Inspired by the Eurorack module of the same name, rebuilt as a plugin without CV. Not affiliated with the module maker.",
 "quick_title": "Quick start",
 "quick": [
   ("Play", "Play <b>one note</b>. It is the root. Or press <b>START DRONE</b> and set ROOT. Hold the key or the drone: the sound swells in over ATTACK."),
   ("Chord", "Turn <b>CHORD</b> to morph through the chord sets, from octaves and power chords to minor triads, min7 / min9, sus2 and a dorian cluster."),
   ("Move", "Set an LFO per bundle and drag its dot in the XY pad to choose what it moves: cutoff, reso, level or pitch."),
   ("Space", "Add <b>REVERB</b> with a long DECAY, then WIDTH and MOVE for stereo drift. Use the LIMITER to make the drone denser."),
 ],
 "first": "First patch: ATTACK 12 s, RELEASE 6 s, CHORD at 0.40, REVERB MIX 0.30. Hold a low C and wait.",
 "sections": [
  ("#e0a800", "DRONE", [
    ("START DRONE", "Starts the drone without a key. ROOT sets the note. With MIDI, the note you play is the root."),
    ("ROOT", "Root note of the drone (C0 to C5)."),
    ("ATTACK", "How long the sound takes to swell in, 0.05 to 120 s."),
    ("RELEASE", "How long it fades out after you let go, 0.05 to 180 s."),
    ("STAGGER", "The root triad enters first, the higher layers follow one after another."),
    ("GLIDE", "Time to glide to a new root when you change the note, 0.01 to 10 s."),
  ]),
  ("#ff7a2f", "TIMBRE", [
    ("CHORD", "Morphs through Just Intonation sets: octaves, power, minor triads, inversions, min7 / min9, sus2, dorian cluster. The name of the current set is shown at the top right of the section."),
    ("SPREAD", "Octave spread of the oscillators."),
    ("TIMBRE", "Position in the wavetable."),
    ("INDEX", "Selects the wavetable."),
    ("ANIMATE", "Automatic slow detune, stereo width and LFO speed in one knob."),
  ]),
  ("#4fb8ee", "BUNDLES A, B, C", [
    ("Bundles", "A = low, B = mid, C = high. Each has 7 oscillators."),
    ("LP / BP / HP", "Filter type of the bundle."),
    ("CUTOFF, RESO", "Filter frequency (40 Hz to 18 kHz) and resonance."),
    ("LEVEL", "Level of the bundle, -24 to +6 dB."),
    ("DETUNE", "Fine detune within the bundle for a supersaw, 0 to 50 cents."),
    ("FM FROM B / FROM C", "FM between the bundles: B modulates A, C modulates B. One slider per path."),
  ]),
  ("#5ee6a8", "LFO (per bundle)", [
    ("RATE", "Speed of the LFO, 0.005 to 2 Hz."),
    ("SHAPE", "Sine, Tri, Ramp or Rand."),
    ("DEPTH", "How strong the LFO works."),
    ("LFO GOES TO", "The XY pad decides the target. Corners: cutoff (top left), reso (top right), level (bottom left), pitch (bottom right). A dot in the middle moves a bit of everything."),
  ]),
  ("#a78bfa", "REVERB, STEREO, OUT", [
    ("MIX / DECAY", "Reverb amount and decay time, 0.5 to 60 s."),
    ("PRE-DELAY", "Pre-delay, 0 to 250 ms."),
    ("TONE", "Left = low-pass, right = high-pass."),
    ("WIDTH", "Widens the drone."),
    ("MOVE", "Adds a slow left / right drift."),
    ("LIMITER", "Threshold with auto gain. Turn it down and the drone gets denser and louder. The ring shows the gain reduction."),
    ("LEVEL", "Master volume."),
  ]),
 ],
 "recipes": [
   ("Slow pad", "ATTACK 20 s, RELEASE 15 s, CHORD 0.1, REVERB DECAY 12 s, MIX 0.3. Play a low note and let it grow."),
   ("Dark drone", "Bundle A only: set B and C LEVEL to -24 dB, CUTOFF A 400 Hz, LFO depth 0.5 on cutoff."),
   ("Bright shimmer", "Bundle C up, CUTOFF 8 kHz, DETUNE 15 ct, ANIMATE 0.6, REVERB TONE to the right."),
   ("Movement", "Different LFO rates per bundle (0.03, 0.05, 0.07 Hz) so the layers never line up."),
 ],
 "tips": [
   "Siren has no CV inputs: MIDI note = root. Sustain pedal and pitch bend (±2 semitones) work.",
   "Long ATTACK values also mean a long wait: play the note early.",
   "FM between the bundles gets harsh quickly. Start with small values.",
   "All parameters can be automated in the DAW and are saved with the project.",
   "Lost in the settings? Right-click the header (or click SIREN): <b>Reset all to default</b> starts fresh, <b>Panic: silence now</b> stops everything at once, including the reverb tail.",
 ],
 "install": install_text("en", "Siren", "siren", "Siren"),
},
"de": {
 "subtitle": "Drohnengenerator",
 "kind": "Instrument für macOS (Apple Silicon)  \xb7  VST3, Audio Unit, CLAP und Standalone",
 "by": "von Dani Koenig",
 "intro": ("Siren ist ein Drohnengenerator. Spiel eine Note, und 21 Oszillatoren schwellen langsam ein, von wenigen Sekunden bis zu Minuten, und blenden aus, wenn du loslässt. "
           "Sie sitzen in drei Bündeln (tief, mittel, hoch), jedes mit eigenem Filter, feinem Detune und LFO. "
           "Akkordsätze in reiner Stimmung, ein langer Hall und ein Limiter am Ende machen aus einer Taste einen riesigen, sich langsam bewegenden Klang."),
 "note": "Inspiriert vom gleichnamigen Eurorack-Modul, als Plugin ohne CV nachgebaut. Keine Verbindung zum Modulhersteller.",
 "quick_title": "Schnellstart",
 "quick": [
   ("Spielen", "<b>Eine Note</b> spielen. Sie ist der Grundton. Oder <b>START DRONE</b> drücken und ROOT einstellen. Taste oder Drohne halten: Der Klang schwillt über ATTACK ein."),
   ("Akkord", "<b>CHORD</b> drehen, um durch die Akkordsätze zu morphen, von Oktaven und Powerchords über Moll-Dreiklänge, min7 / min9 und sus2 bis zu einem dorischen Cluster."),
   ("Bewegung", "Pro Bündel einen LFO einstellen und seinen Punkt im XY-Pad ziehen, um zu wählen, was er bewegt: Cutoff, Reso, Level oder Pitch."),
   ("Raum", "<b>REVERB</b> mit langem DECAY zumischen, dazu WIDTH und MOVE für Stereo-Drift. Mit dem LIMITER wird die Drohne dichter."),
 ],
 "first": "Erster Patch: ATTACK 12 s, RELEASE 6 s, CHORD auf 0.40, REVERB MIX 0.30. Ein tiefes C halten und warten.",
 "sections": [
  ("#e0a800", "DRONE", [
    ("START DRONE", "Startet die Drohne ohne Taste. ROOT stellt die Note ein. Mit MIDI ist die gespielte Note der Grundton."),
    ("ROOT", "Grundton der Drohne (C0 bis C5)."),
    ("ATTACK", "Wie lange der Klang zum Einschwellen braucht, 0.05 bis 120 s."),
    ("RELEASE", "Wie lange er nach dem Loslassen ausblendet, 0.05 bis 180 s."),
    ("STAGGER", "Der Grund-Dreiklang setzt zuerst ein, die höheren Lagen folgen nacheinander."),
    ("GLIDE", "Zeit, um zu einem neuen Grundton zu gleiten, 0.01 bis 10 s."),
  ]),
  ("#ff7a2f", "TIMBRE", [
    ("CHORD", "Morpht durch Sätze in reiner Stimmung: Oktaven, Power, Moll-Dreiklänge, Umkehrungen, min7 / min9, sus2, dorischer Cluster. Der Name des aktuellen Satzes steht oben rechts im Abschnitt."),
    ("SPREAD", "Oktav-Verteilung der Oszillatoren."),
    ("TIMBRE", "Position in der Wavetable."),
    ("INDEX", "Wählt die Wavetable."),
    ("ANIMATE", "Automatisches langsames Detune, Stereobreite und LFO-Tempo in einem Regler."),
  ]),
  ("#4fb8ee", "BÜNDEL A, B, C", [
    ("Bündel", "A = tief, B = mittel, C = hoch. Jedes hat 7 Oszillatoren."),
    ("LP / BP / HP", "Filtertyp des Bündels."),
    ("CUTOFF, RESO", "Filterfrequenz (40 Hz bis 18 kHz) und Resonanz."),
    ("LEVEL", "Pegel des Bündels, -24 bis +6 dB."),
    ("DETUNE", "Feines Detune im Bündel für einen Supersaw, 0 bis 50 Cent."),
    ("FM FROM B / FROM C", "FM zwischen den Bündeln: B moduliert A, C moduliert B. Ein Schieber pro Strecke."),
  ]),
  ("#5ee6a8", "LFO (pro Bündel)", [
    ("RATE", "Tempo des LFOs, 0.005 bis 2 Hz."),
    ("SHAPE", "Sine, Tri, Ramp oder Rand."),
    ("DEPTH", "Wie stark der LFO wirkt."),
    ("LFO GOES TO", "Das XY-Pad bestimmt das Ziel. Ecken: Cutoff (oben links), Reso (oben rechts), Level (unten links), Pitch (unten rechts). Ein Punkt in der Mitte bewegt von allem etwas."),
  ]),
  ("#a78bfa", "REVERB, STEREO, OUT", [
    ("MIX / DECAY", "Hallanteil und Ausklingzeit, 0.5 bis 60 s."),
    ("PRE-DELAY", "Pre-Delay, 0 bis 250 ms."),
    ("TONE", "Links = Tiefpass, rechts = Hochpass."),
    ("WIDTH", "Verbreitert die Drohne."),
    ("MOVE", "Fügt eine langsame Links-Rechts-Bewegung hinzu."),
    ("LIMITER", "Threshold mit Auto-Gain. Nach unten gedreht wird die Drohne dichter und lauter. Der Ring zeigt die Gain Reduction."),
    ("LEVEL", "Gesamtlautstärke."),
  ]),
 ],
 "recipes": [
   ("Langsames Pad", "ATTACK 20 s, RELEASE 15 s, CHORD 0.1, REVERB DECAY 12 s, MIX 0.3. Eine tiefe Note spielen und wachsen lassen."),
   ("Dunkle Drohne", "Nur Bündel A: LEVEL von B und C auf -24 dB, CUTOFF A 400 Hz, LFO-Depth 0.5 auf Cutoff."),
   ("Helles Schimmern", "Bündel C hoch, CUTOFF 8 kHz, DETUNE 15 ct, ANIMATE 0.6, REVERB TONE nach rechts."),
   ("Bewegung", "Verschiedene LFO-Tempi pro Bündel (0.03, 0.05, 0.07 Hz), damit sich die Lagen nie decken."),
 ],
 "tips": [
   "Siren hat keine CV-Eingänge: MIDI-Note = Grundton. Sustain-Pedal und Pitch Bend (±2 Halbtöne) funktionieren.",
   "Lange ATTACK-Werte heissen auch lange warten: Die Note früh spielen.",
   "FM zwischen den Bündeln wird schnell scharf. Mit kleinen Werten anfangen.",
   "Alle Parameter lassen sich in der DAW automatisieren und werden mit dem Projekt gespeichert.",
   "Zu viel verstellt? Rechtsklick auf die Kopfzeile (oder Klick auf SIREN): <b>Reset all to default</b> beginnt von vorn, <b>Panic: silence now</b> stoppt sofort alles, auch den Hall-Schwanz.",
 ],
 "install": install_text("de", "Siren", "siren", "Siren"),
},
}
