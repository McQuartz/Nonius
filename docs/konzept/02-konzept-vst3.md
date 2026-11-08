# Konzept: Mix-Check VST3 (Arbeitstitel "Probe")

Stand: 08.11.2026 · Genre: Hip-Hop/Rap · Ziel: besseres Ergebnis, kein Lern-Tool, kein Ersatz für FabFilter & Co.

## Grundidee
Ein reines **Analyse-Plugin, das keinen Sound verändert**. Man zieht es an beliebige Stellen in Ableton. Es misst das Signal, nicht die Geräte. Deshalb ist die Reihenfolge der Kette egal.

Die Instanzen kennen sich gegenseitig (gemeinsamer Speicher im selben Prozess). Daraus ergeben sich drei Modi:

1. **Spur-Check:** Eine Instanz am Ende der Vocal-Kette. Vergleich mit einem Hip-Hop-Zielprofil ("Rap Lead Vocal", "Adlibs", "808", "Drum-Bus", "Beat-Bus", "Master").
2. **Vorher/Nachher:** Zwei Instanzen um ein Gerät oder einen Kettenabschnitt herum. Zeigt, was das Gerät *wirklich* macht: zu viel Kompression, Höhen weg, Zischlaute verstärkt, Sättigung macht 3 kHz hart.
3. **Vocal gegen Beat:** Die Instanz auf dem Vocal sieht die Instanz auf dem Beat-Bus und meldet Überdeckung, z. B. "Beat verdeckt Vocal bei 1,5–3 kHz → Beat dort 2 dB dynamisch absenken (Sidechain vom Vocal)".

## Was angezeigt wird
- Kurze Liste mit max. 3–5 Problemen, sortiert nach Wichtigkeit, jeweils mit **konkretem Fix** (Frequenz, dB, Q, Attack/Release-Richtung). Den Fix setzt man dann im eigenen FabFilter um.
- Ampel pro Kategorie: Tonalität · Mumpf (200–500 Hz) · Härte (2–5 kHz) · Zischlaute (5–10 kHz) · Dynamik/Crest · Lautheit · Stereo/Mono · Clipping/Headroom.
- "Freeze"-Knopf: einen Abschnitt (z. B. 8 Takte Hook) analysieren und das Ergebnis festhalten statt ständig flackernder Werte.

## Zielprofile
Werden aus Referenztracks gemessen: Stems, Acapellas oder ganze Songs. Eigene Referenzen kann man per Drag & Drop einlesen ("so soll mein Vocal sitzen").

## Technik
- C++ mit **JUCE**, Ausgabe VST3 für Windows.
- Analyse: FFT in Terz-/Bark-Bändern, LUFS (kurz/integriert), Crest-Factor, Transienten, Sibilanz-Detektor, Korrelation, spektrale Maskierung zwischen Instanzen.
- Regelwerk (keine Blackbox-KI) im ersten Schritt, damit die Hinweise nachvollziehbar sind. Optional später ML oder KI-Erklärung.
- Bauen: entweder Visual Studio auf deinem PC oder automatisch über GitHub Actions (dann lädst du nur die fertige .vst3 runter).

## MVP (Version 1)
1. Spur-Check für Rap-Vocal mit 1 eingebautem Profil + eigenem Referenz-Import
2. Vorher/Nachher-Modus
3. Vocal gegen Beat (Maskierung)
4. Problemliste mit konkreten Fixes

Später: weitere Profile (808, Drums, Master), Max-for-Live-Begleiter, der Ableton-Geräteparameter ausliest, KI-Erklärung.

## Ergänzung: Aufnahme-Check (Modus "Aufnahme", ganz vorne in der Kette)
Wunsch von Ace: Die Aufnahmesituation soll berücksichtigt werden (Mikros: Rode NTK = Röhren-Kondensator, SM7B = dynamisch; Raum hallt stark).

Messbar am Rohsignal:
- **Pegel/Gain-Staging:** Peaks, Durchschnitt, Clipping. Ziel grob Peaks −10 bis −6 dBFS, kein Clipping.
- **Raumhall:** Ausklingen nach Phrasenende messen (Nachhallzeit-Schätzung). Hinweis, dass Kompression den Raum hochholt.
- **Rauschen/Störgeräusche:** Grundrauschen in den Pausen (wichtig beim SM7B mit viel Gain).
- **Nahbesprechung/Plosive:** Bass-Wummern, Popps.
- **Mikro-Profil wählbar (NTK / SM7B / anderes):** passt die Erwartungen an (NTK heller und nimmt mehr Raum auf, SM7B dunkler, braucht viel Gain, weniger Raum).

Ausgabe: Hinweise für die *nächste Aufnahme* (z. B. "Decke hinter dich hängen", "SM7B statt NTK in diesem Raum", "Gain +6 dB") und für den *Mix* (z. B. "De-Reverb vor dem Kompressor", "weniger komprimieren, sonst kommt der Raum hoch"). Die Spur-Check-Hinweise berücksichtigen das Ergebnis.
