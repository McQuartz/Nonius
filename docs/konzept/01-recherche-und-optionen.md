# Mix-Feedback in Ableton: Recherche & Optionen

Stand: 08.11.2026 · Ziel: bessere Mixes (zuerst Vocals, dann Beat), Feedback direkt an der Stelle in der Kette, wo man gerade arbeitet.

## 1. Was es schon gibt

### a) "Assistenten", die analysieren UND einstellen
| Tool | Was es macht | Für dich relevant? |
|---|---|---|
| **iZotope Nectar 4** | Vocal Assistant: hört die Stimme, baut eine Vocal Chain (EQ, Comp, De-Ess, Sättigung…) | Sehr – genau dein Haupt-Usecase. Gut zum *Lernen*: anschauen, was er macht, und warum. |
| **iZotope Neutron 5** | Mix Assistant für einzelne Spuren/Busse, Unmask (Frequenz-Überdeckung zwischen zwei Spuren, z. B. Vocal vs. Beat) | Ja, v. a. "Unmask" für Vocal gegen Beat. |
| **iZotope Tonal Balance Control 3** | Vergleicht Frequenzbalance mit Zielkurven oder eigenen Referenzsongs | Sehr gut für den Master-/Busvergleich. |
| **Sonible smart:EQ 4 / smart:comp 2 / pure:-Serie** | Lernt Signal, schlägt EQ/Comp vor, Gruppen-Modus für mehrere Spuren | Ja, als "Aufräum-EQ"-Hilfe. |

### b) Reine Feedback-/Analyse-Tools (sagen dir, was nicht passt)
| Tool | Was es macht |
|---|---|
| **Mastering The Mix REFERENCE 3** | Referenzsong laden, "Trinity Display" + konkrete Hinweise, was zu tun ist (z. B. "Bass +2 dB"). |
| **Mastering The Mix MIXROOM / BASSROOM / LEVELS / EXPOSE** | Zielkurven für Vocals/Instrumente (MIXROOM), Bass-Balance, Check auf Lautheit, Stereo, Dynamik, Phase. |
| **RoEx Mix Check Studio** (Web, kostenlos) | Bounce hochladen → Bericht, was am Mix falsch ist und wie man es fixt. Kein Plugin, aber genau die "sag mir was nicht geil ist"-Idee. |
| **Youlean Loudness Meter**, **SPAN** (Voxengo, gratis) | Klassisches Metering: LUFS, Spektrum, Korrelation. |

### c) Ableton-intern / Max for Live
- Ableton hat eingebaut: Spectrum, Tuner, Utility (Mono-Check), EQ Eight mit Analyzer.
- M4L-Geräte für Analyse/Metering gibt es viele (Loudness-Meter, Multi-Analyser usw.), aber kaum etwas, das *bewertet*.

### d) KI-Anbindung an Ableton
- Es gibt mehrere **Ableton-MCP-Server** (z. B. ahujasid/ableton-mcp, LivePilot), mit denen ein KI-Assistent wie Claude die Session lesen und steuern kann (Spuren, Geräte, Parameter). Das ist ein interessanter Baustein für Option 4 unten.

**Fazit Recherche:** Es gibt gute Tools, die *Frequenzbalance mit Referenz* vergleichen und *Vorschläge* machen. Was fehlt: ein Tool, das **die ganze Kette** versteht ("dein zweiter Kompressor drückt 9 dB, das ist zu viel für diesen Stil", "die Sättigung bringt Härte bei 3 kHz") und dir das **erklärt**, sodass du dabei lernst.

## 2. Optionen für ein eigenes Werkzeug

### Option 1 – Fertige Tools klug kombinieren (sofort, kein Code)
Nectar 4 oder smart:EQ zum Lernen + REFERENCE 3 / Tonal Balance auf dem Master + feste Check-Routine (Mono, Lautheit, Referenz A/B). Kosten ca. 100–300 €. Schnellster Gewinn.

### Option 2 – "Mix-Coach" als Max for Live Device
Ein M4L-Audio-Effekt, den man an beliebiger Stelle der Kette einfügt. Misst live: Spektrum (in Bändern), Crest-Factor/Dynamik, LUFS, Sibilanz (5–9 kHz), Mumpf (200–500 Hz), Härte (2–5 kHz), Stereo/Mono. Vergleicht mit Zielprofilen ("Rap-Vocal", "Pop-Vocal", "808", "Drum-Bus") und zeigt Ampeln + Klartext.
- **Plus:** Läuft nur in Ableton (genau dein Workflow), schnell zu bauen, kann über die Live-API auch *die anderen Geräte der Spur lesen* (z. B. Kompressor-Gain-Reduction, EQ-Einstellungen).
- **Minus:** Max-Programmierung ist eigenwillig; DSP-Leistung begrenzt; braucht Live Suite oder M4L.

### Option 3 – Eigenes VST3 (JUCE, C++)
Gleiche Idee wie Option 2, aber als echtes Plugin.
- **Plus:** Profi-Qualität, läuft in jeder DAW, mehrere Instanzen können sich gegenseitig "sehen" (z. B. Vocal-Instanz kennt Beat-Instanz → Maskierung erkennen).
- **Minus:** Deutlich mehr Aufwand (Wochen statt Tage), sieht nicht ins Ableton-Gerät nebenan.

### Option 4 – KI-Coach außerhalb des Plugins (Messen + Erklären trennen)
Ein schlankes Mess-Plugin (M4L oder VST) schreibt Messwerte raus; zusätzlich liest ein Ableton-MCP-Server die Gerätekette. Ein KI-Assistent bekommt beides + optional einen kurzen Bounce und gibt **erklärendes Feedback in Sprache**: "Bei Vocal 1 ist nach dem 2. EQ zu viel 250 Hz übrig; der Kompressor arbeitet mit Ratio 8:1 und 10 dB GR – probier 4:1 und 4 dB."
- **Plus:** Lerneffekt am größten, Feedback kennt Kontext (Genre, Referenz, ganze Kette).
- **Minus:** Kein Echtzeit-Feedback im Sinne von Millisekunden, eher "Check-Knopf"; braucht etwas Setup.

### Option 5 – Prozess statt Technik
Referenz-Workflow, Checkliste pro Schritt, Gain-Staging-Regeln, gelegentlich Mix an RoEx Mix Check Studio schicken. Kostet nichts und hilft sofort – lässt sich mit jeder Option kombinieren.

## 3. Meine Empfehlung
1. **Jetzt:** Option 1 + 5 ausprobieren (z. B. Nectar-Trial + REFERENCE-Trial + RoEx-Check), um zu sehen, welche Art Feedback dir wirklich hilft.
2. **Dann bauen:** Option 2 als Prototyp ("Vocal-Coach" M4L-Device mit Zielprofilen und Klartext-Hinweisen). Kleinster Bau, direkt in deinem Workflow.
3. **Später:** Option 4 draufsetzen, wenn du erklärende KI-Kommentare zur ganzen Kette willst.

## 4. Offene Fragen an dich
- Hast du **Live Suite** (also Max for Live)?
- Welche **Genres/Referenzen**? (Zielprofile hängen davon ab.)
- Willst du eher **Ampel/Live-Anzeige** beim Drehen oder eher einen **"Check"-Knopf mit ausführlicher Erklärung**?
- Besitzt du schon Tools wie Nectar, Neutron, Ozone oder Sonible?

## Quellen
- iZotope Neutron 5 (MusicRadar): https://www.musicradar.com/music-tech/plugins/izotope-updates-neutron-5-with-three-new-plugins-and-makes-its-ai-powered-mix-assistant-faster-and-smarter
- Neutron 5 im Test gegen Mix-Engineer (MusicRadar): https://musicradar.com/music-tech/can-ai-mix-better-than-a-real-mix-engineer-we-put-izotopes-ai-powered-neutron-5-to-the-test-to-find-out
- REFERENCE 3 Review (MusicTech): https://musictech.com/reviews/plug-ins/mastering-the-mix-reference-3-review-2/
- Tonal Balance Control 3 (Mixdown): https://mixdownmag.com.au/news/izotope-tonal-balance-control-3-lets-you-use-any-song-as-a-reference-including-from-spotify-and-youtube/
- RoEx Mix Check Studio: https://mixcheckstudio.roexaudio.com/ · MusicRadar: https://musicradar.com/news/mix-check-studio-free-ai-tool
- Ableton MCP: https://mcpservers.org/servers/ahujasid/ableton-mcp · LivePilot: https://claudemarketplaces.com/mcp/dreamrec/livepilot
