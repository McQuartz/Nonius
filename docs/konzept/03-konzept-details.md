# Mix-Check: Konzept-Details (Stand 08.11.2026)

Ergänzt 02-konzept-vst3.md. Antworten auf Aces Fragen zu Hinweisen, Referenzen, Stilen und Ableton-Ablauf.

## 1. Wie Hinweise entstehen
- **Fester Katalog von Problem-Typen** (Ziel: 40–60 Checks), jeder mit
  - Detektor (was wird gemessen, z. B. Energie 200–500 Hz relativ zum Ziel),
  - Schwellwerten, die von Rolle (Lead, Double, Adlib, Kick…), Stil und Mikro abhängen,
  - Schweregrad (OK / Achtung / Problem) und Priorität,
  - vorformuliertem Text mit Platzhaltern: "Mumpf bei {f1}–{f2} Hz (+{dB} dB über Ziel) → −{fix} dB bei {fc} Hz, Q {q}".
- Angezeigt werden immer nur die wichtigsten 3–5. Keine freie KI-Formulierung in Version 1: gleiche Messung = gleicher Hinweis, nichts Erfundenes.
- Gruppen im Katalog: Aufnahme (Pegel, Clipping, Raum, Rauschen, Popps) · Tonalität (Sub, Wummern, Mumpf, Nasal, Härte, Zischlaute, Air) · Dynamik (zu viel/zu wenig Kompression, Pumpen, Transienten weg) · Sättigung/Distortion (Härte, Aliasing-Hinweis) · Stereo/Mono (Phasenprobleme, Doubles zu breit/eng) · Beziehungen (Beat verdeckt Vocal, Vocal zu leise/laut, 808 vs. Kick, 808 vs. Vocal-Tiefen, Doubles lauter als Lead…) · Gesamt (Lautheit, Headroom).
- Hinweise sind Vorschläge; Beispiel Raum: zuerst "Raum dämpfen (Decken/Absorber hinter und neben dir)", danach "alternativ SM7B", für den Mix "De-Reverb vor dem Kompressor".

## 2. Referenzen richtig analysieren (keine Abkürzungen)
- **Problem:** Ein fertiger Song ist ein ganzer Mix. Eine einzelne Vocal-Spur mit einem ganzen Song zu vergleichen, ergibt falsche Ergebnisse.
- **Lösung:** Beim Einlesen wird jede Referenz per **Stem-Trennung** (Open-Source-Modell, z. B. Demucs) in Vocals, Drums, Bass, Rest zerlegt. Dann wird verglichen:
  - Vocal-Spur ↔ Vocal-Stem der Referenz,
  - Vocal ↔ Beat-Verhältnis (Lautstärke, Überdeckung) ↔ dasselbe Verhältnis in der Referenz,
  - Master ↔ ganzer Song.
- Weitere Regeln: Lautheit vorher angleichen (LUFS), Stille/Intro/Outro ignorieren, Langzeit-Spektrum in feinen Bändern mit Glättung, Hook und Strophe getrennt, Mittelwert + Streuung über alle Songs eines Ordners (ein Ausreißer-Song verbiegt das Ziel nicht).
- Analyse läuft einmal offline beim Einlesen (dauert pro Song ca. 0,5–2 Min.), Ergebnis wird gespeichert.

## 3. Referenz-Bibliothek
- Ein Ordner, z. B. `Dokumente\Mix-Check\Referenzen\`, Unterordner = Stile: `Trap\`, `Drill UK\`, `Boom Bap\`, eigene Ordner wie `Mein Sound\`.
- Plugin scannt den Ordner, analysiert neue Songs automatisch, zeigt alles im Dropdown. Auswahl: ganzer Stil-Ordner oder einzelner Song.

## 4. Stilrichtungen: zwei Ebenen
- **Basis (immer aktiv):** allgemeine Regeln, die für jeden Rap-Mix gelten (kein Clipping, keine extremen Resonanzen, Zischlaute im Rahmen, Mono-tauglich, Vocal verständlich).
- **Stil (gewählt):** grobe Familie + genauere Richtung, z. B.
  - Trap: Trap, Melodic/Cloud, Rage, Plugg
  - Drill: UK Drill, NY/Brooklyn Drill
  - Boom Bap: klassisch, Lo-Fi/Dusty, Modern Boom Bap
  - Weitere: Old School, Jersey, Phonk, Afro-/Pop-Rap, Deutschrap Straße, Deutschrap Pop
- Eingebaute Profile sind Startwerte; genauer wird es mit eigenen Referenz-Ordnern. Künstler als Richtung = Songs des Künstlers in einen Ordner legen.

## 5. Wie es in Ableton funktioniert
- **Pro Spur eine Instanz** ("Sonde"), klein, ohne dass man sie öffnen muss. Rolle wird pro Instanz gewählt (Lead, Double L/R, Adlib, Kick, Snare, 808, Beat 2-Track, Bus…), wo möglich automatisch aus dem Spurnamen erraten.
- **Eine Zentrale auf dem Master** zeigt alle Sonden auf einen Blick: Liste aller Spuren mit Ampel, Klick auf eine Spur → Details. Spur-Check und Beziehungen (Vocal gegen Beat) stehen gleichzeitig nebeneinander, kein Durchklicken.
- Fenster bleibt offen: in Ableton unter Einstellungen → Plug-Ins "Mehrere Plug-In-Fenster" an und "Plug-In-Fenster automatisch ausblenden" aus. Dann bleibt die Zentrale offen, während man auf anderen Spuren arbeitet.
- Beat als 2-Track (eine Stereo-Spur, typischer Recording-Fall) und Beat als Stems (Kick, Snare, 808 …) werden beide unterstützt.

## 6. UI
- Später, wenn der Ablauf steht. Vorgabe: soll nicht nach KI aussehen.
