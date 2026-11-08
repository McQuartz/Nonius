# Mix-Check: Technisches Konzept (Stand 08.11.2026)

Ziel dieses Dokuments: Alles, was "im Hintergrund" passiert, sauber festlegen, bevor gebaut wird. Ergänzt 02 (Grundidee) und 03 (Ablauf).

---

## 1. Was die Profis machen und was wir übernehmen

| Tool | Technik (laut Hersteller-Doku) | Was wir übernehmen |
|---|---|---|
| iZotope Tonal Balance Control | Misst die *Form* des Spektrums, unabhängig von der Lautstärke. Mittelt über mehrere Sekunden bis zum ganzen Song, geglättet in Zeit und Frequenz. Zeigt keine Ziel-Linie, sondern einen **Bereich** (typische Streuung über tausende Songs). 4 grobe Bänder (<250 Hz, 250 Hz–2 kHz, 2–8 kHz, >8 kHz) plus feine Ansicht. Zusätzlich Crest-Factor im Bass (Spitze minus Durchschnitt) für "zu dynamisch / zu komprimiert". Eigene Ziele aus Datei oder ganzem Ordner. | Bereiche statt Linien. Lautstärke-unabhängiger Vergleich. Lange Mittelung. Ordner → ein gemeinsames Ziel. Crest-Factor pro Band. |
| | **Wichtig:** iZotope schreibt, dass Genre-Etiketten bei ihren Kurven *wenig Unterschied* gemacht haben. Sie haben am Ende nur 3 grobe Kurven. | Spricht für "Standard" als Normalfall (siehe Abschnitt 6). |
| iZotope Neutron Masking Meter / Unmask | Maskierung = **Lautheitsverlust**: wie laut man das Vocal allein wahrnimmt minus wie laut man es im Beat noch wahrnimmt (Gehörmodell mit Außen-/Mittelohr, Phon). Gezählt pro kritischem Band (32 Bänder), über Fenster von 3 s; ein "Treffer" nur, wenn der Verlust über einem Band-Grenzwert liegt. Braucht einen Fader-Ausgleich, weil das Plugin vor dem Fader sitzt. | Gleiches Prinzip für "Beat verdeckt Vocal": Gehör-Bänder, 3-s-Fenster, Zähl-Statistik, Fader-Ausgleich. |
| Mastering The Mix MIXROOM | Ziel aus bis zu 20 Referenzen, empfohlen nur Hook/Drop. Ignoriert Abweichungen unter ±0,5 dB. Erste Vorschläge nach ca. 3 s Analyse. | Mindest-Abweichung, unter der nichts gemeldet wird. Hook getrennt von Strophe. |
| Mastering The Mix REFERENCE 3 | Abweichung zur Referenz pro Band, ±3 dB gilt als "gleiche Liga". Fenster 3 s / 10 s / durchgehend. Text-Anweisungen wie "Bass um 4 dB boosten". | Toleranz in dB-Bereichen, wählbare Messfenster, konkrete Anweisungen in dB. |

Was keiner dieser Hersteller offenlegt: FFT-Größen, genaue Schwellwerte, wie Vorschläge berechnet werden. Das legen wir selbst fest (unten) und kalibrieren es mit Tests (Abschnitt 10).

---

## 2. Grundregeln für das Plugin

1. **Ton bleibt unangetastet.** Das Audio geht bitgenau durch. Keine Latenz.
2. **Im Echtzeit-Teil passiert fast nichts.** Der Audio-Thread kopiert die Samples nur in einen Ringpuffer (lock-free, ohne Speicher-Reservierung). Alle Messungen laufen in einem eigenen Analyse-Thread. So kann das Plugin keine Knackser verursachen.
3. **Gemessen wird nur, wenn Ableton läuft** (Transport "Play"). Beim Export, der schneller als Echtzeit läuft, wird die Analyse pausiert.
4. **Jede Aussage hat eine Sicherheit.** Zu wenig Material (z. B. < 8 s mit Gesang) → "zu wenig Daten" statt eines falschen Hinweises.

---

## 3. Mess-Kette (pro Instanz)

Vorverarbeitung: auf 48 kHz intern, Mono-Summe + Mitte/Seite, FFT 4096 Punkte, Hop 1024 (ca. 21 ms), Hann-Fenster.

**Aktivitätserkennung zuerst:** Für Vocals eine Sprach-/Gesangs-Erkennung (Pegel + spektrale Merkmale), für Instrumente ein Pegel-Gate. Pausen werden separat ausgewertet (Rauschen, Hall), nur aktive Teile zählen für Klang-Messungen. Sonst würden Pausen das Ergebnis verfälschen.

| Messung | Methode | Wofür |
|---|---|---|
| Lautheit | ITU-R BS.1770 (K-Filter): Momentary 400 ms, Short-term 3 s, Integrated mit Gating; LRA | Lautheit, Verhältnis Vocal/Beat, Dynamik |
| True Peak | 4-fach Oversampling | Headroom, Clipping am Master |
| Spektrum | Leistung in ca. 1/6-Oktav-Bändern (20 Hz–20 kHz), Langzeit-Mittel über aktive Frames, lautheitsnormiert, geglättet; zusätzlich Streuung pro Band über die Zeit | Tonalität: Wummern, Mumpf, Nasal, Härte, Air |
| Resonanzen | Feines Spektrum minus geglättetes Spektrum; schmale Spitzen > X dB, die über die Zeit stehen bleiben | "pfeifende" Frequenzen, Raummoden, Mikro-Eigenheiten |
| Dynamik | Crest-Factor gesamt und pro Band, PLR, Short-term-Streuung, Transienten-Stärke | "zu stark komprimiert / zu wenig", "Pumpen", "Transienten weg" |
| Zischlaute | S-Ereignisse erkennen (kurze Frames mit Energie-Schwerpunkt 5–10 kHz), Pegel der S-Ereignisse relativ zu den Vokalen | "Zischlaute X dB zu laut" → De-Esser-Empfehlung |
| Zeitlich wechselnde Probleme | Wie oft (Prozent der Zeit) ein Band über dem Ziel liegt | Entscheidet zwischen "statischer EQ" und "dynamischer EQ" |
| Stereo | Korrelation, Mitte/Seite pro Band, Mono-Verlust | Phasenprobleme, Doubles zu breit/eng, Bass nicht mono |

**Nur im Modus Aufnahme-Check (Rohspur):**

| Messung | Methode |
|---|---|
| Pegel/Gain | Peak, RMS, Anteil der Zeit nahe 0 dBFS |
| Clipping | Mehrere aufeinanderfolgende Samples an derselben Grenze ("flache Spitzen"). Erkennt auch Clipping, das schon im Interface passiert ist und später leiser gemacht wurde |
| Raumhall | Blinde Nachhall-Schätzung aus den Ausklingphasen nach Phrasenende (Maximum-Likelihood-Verfahren, Literatur: Löllmann et al.). Ausgabe als grobe Stufe (trocken / leicht / deutlich / stark) mit Sicherheit, nicht als exakte Zahl |
| Rauschen/Brummen | Grundrauschen in Pausen; Brumm-Erkennung bei 50 Hz + Obertönen |
| Popps | Kurze Bass-Stöße < 150 Hz mit schnellem Einsatz |
| Nahbesprechung | Bass-Anstieg relativ zum Mikro-Profil |
| DC-Offset | Gleichanteil |

---

## 4. Vergleich mit dem Ziel

- **Ein Ziel ist eine Verteilung, keine Linie:** pro Messung und Band Median plus 10.–90. Perzentil, getrennt nach Rolle (Lead, Double, Adlib, Kick, 808, Beat…) und, wenn möglich, nach Teil (Hook/Strophe).
- **Abweichung** wird in Einheiten der Streuung gemessen. Gemeldet wird nur, wenn:
  1. außerhalb des Bereichs,
  2. mehr als eine Mindestabweichung (Start: 1 dB, wird kalibriert),
  3. über mehrere Messfenster stabil (keine flackernden Hinweise),
  4. genug Daten vorhanden.
- **Priorität** = Größe der Abweichung × wie gut das Ohr dort hört (Gehörkurve) × Sicherheit. Die Top 3–5 werden angezeigt.
- **Fix berechnen:** Bereich der Abweichung → Mittenfrequenz und Breite (→ Q). Gain = ca. 70 % der Abweichung (lieber vorsichtig), maximal 6 dB. Tritt das Problem nur zeitweise auf (siehe "zeitlich wechselnd"), wird ein dynamischer EQ/De-Esser vorgeschlagen statt eines festen EQ.
- **Hinweis-Katalog:** fester Katalog (siehe 03), Texte mit Platzhaltern. Wenn mehrere Hinweise dieselbe Ursache haben (z. B. Mumpf + zu wenig Air = insgesamt zu dunkel), wird ein zusammenfassender Hinweis gezeigt statt zwei einzelner.

---

## 5. Referenzen einlesen

1. **Bibliothek:** Ordner `Dokumente\Mix-Check\Referenzen\<Stil>\`. Formate: WAV, AIFF, FLAC, MP3 (über Windows), OGG.
2. **Hilfsprogramm** "Mix-Check Library" (eigene .exe, wird vom Plugin gestartet). Grund: Stem-Trennung braucht viel Speicher und Rechenzeit. Läuft sie außerhalb von Ableton, kann sie Ableton weder verlangsamen noch zum Absturz bringen.
3. **Ablauf pro Song:** dekodieren → auf 48 kHz → Lautheit angleichen → **Stem-Trennung mit HTDemucs** (Open Source, MIT-Lizenz, läuft in C++ über ONNX Runtime; laut Mixxx-Projekt ca. 20 s Rechenzeit pro Minute Musik auf der CPU, also ca. 1–1,5 Min. pro Song) → Messung wie in Abschnitt 3 auf jedem Stem und auf dem ganzen Song → Ergebnis speichern (nur Zahlen, keine Audiodaten).
4. **Hook/Strophe:** automatische Teil-Erkennung über Lautheit und Klangwechsel. Hook und Strophe werden getrennt gespeichert.
5. **Ordner-Ziel:** Mittelwert + Streuung über alle Songs, Ausreißer gedämpft (robuste Statistik).
6. **Zwischenspeicher:** pro Datei mit Prüfsumme. Neue Datei → wird analysiert. Gelöschte Datei → fliegt raus. Neue Plugin-Version mit geänderter Messung → wird neu analysiert.
7. **Bekannte Schwäche:** Stem-Trennung ist nicht perfekt; z. B. landen Hi-Hats teilweise im Vocal-Stem. Deshalb werden die Bereiche bei den Höhen etwas breiter angesetzt, und die Trennqualität wird pro Song geschätzt. Schlecht getrennte Songs zählen weniger.

---

## 6. Stile und Standard (Antwort auf deine Frage)

- **Standard ist der Normalfall.** Man muss keine Richtung wählen.
- Richtung ist optional, für Fälle wie "Boom Bap soll wärmer sein".
- Eigene Referenz-Ordner sind die genaueste Option und ersetzen eine Richtung.
- **Ehrlicher Punkt:** Ein guter Standard braucht Daten aus vielen Songs. Variante A: Startwerte aus der Literatur (z. B. Katz, Tonal-Balance-Prinzip) und ein Standard aus deinen eigenen Referenzen. Variante B: Du legst einmal 20–50 Songs ab, die du gut findest, und daraus wird dein Standard. Mischbar.

---

## 7. Instanzen verbinden

- **Gemeinsamer Speicher im selben Programm** (statisches Objekt, JUCE `SharedResourcePointer`). Ableton lädt Plugins im selben Prozess, eine Plugin-Sandbox wie bei Bitwig gibt es nicht. Falls doch einmal keine Verbindung zustande kommt: Hinweis "nicht verbunden", jede Instanz funktioniert dann allein weiter.
- Jede Instanz meldet: Rolle, Spurname (Ableton liefert ihn über VST3; bekannte Eigenheit: Name und Farbe kommen in getrennten Meldungen, also jeweils den letzten nicht-leeren Wert merken), Messwerte der letzten Sekunden mit Zeitstempel aus Abletons Zeitlinie.
- **Zeitabgleich:** Maskierung wird über 3-s-Fenster ausgewertet. Kleine Verzögerungen zwischen Spuren (Latenzausgleich) spielen dabei keine Rolle.
- **Fader-Problem:** Das Plugin sitzt vor Abletons Lautstärke-Fader und Panorama. Es weiß also nicht, wie laut die Spur am Ende wirklich im Mix ist. Lösungen:
  - v1: Die Instanz auf dem Master misst die echte Summe. Daraus und aus den Spur-Instanzen wird der Fader-Wert jeder Spur geschätzt (Vergleich der Pegelverläufe). Zusätzlich kann man im Plugin einen Fader-Ausgleich von Hand eintragen, wie bei Neutron.
  - Tipp im Handbuch: Mix-Check als letztes Gerät, Lautstärke lieber mit Utility davor als mit dem Fader.
- **Stummgeschaltete Spuren:** In Ableton laufen Plugins auf stummen Spuren weiter. Lösung über den Abgleich mit dem Master: Spuren, die dort nicht ankommen, werden als "stumm" erkannt und nicht mitgezählt.

---

## 8. Fenster in Ableton (Antwort auf deine Frage)

- Abletons Standard-Einstellung ("Plug-In-Fenster automatisch ausblenden" an) macht genau das, was du willst: Du wählst eine Spur aus, und deren geöffnete Plugin-Fenster erscheinen, die der anderen Spur verschwinden. Du musst nichts umstellen.
- Deshalb zeigt **jede Instanz alles Wichtige für ihre Spur selbst**: Spur-Check, Beziehung zum Beat und eine kleine Liste "andere Spuren mit Problemen". Eine Zentrale auf dem Master ist optional für den Gesamtüberblick.

---

## 9. Sonderfälle (Checkliste)

| Fall | Verhalten |
|---|---|
| Transport steht | keine Analyse, letzte Ergebnisse bleiben stehen |
| Loop über 8 Takte | normal, zählt wie mehrfaches Hören; "8 Takte prüfen" nutzt BPM und Taktposition von Ableton |
| Export/Freeze (schneller als Echtzeit) | Analyse pausiert |
| Samplerate 44,1/48/88,2/96 kHz, wechselnde Puffergrößen | intern auf 48 kHz, alles läuft weiter |
| Mono-Spur / Stereo-Spur | beides; Mono auf Stereo-Spur wird erkannt |
| Spur ist still | "keine Daten" statt Fehlhinweis |
| Plugin auf Gruppe/Bus | Rolle "Bus" wählbar, andere Ziele |
| Mehrere Lead-Vocal-Spuren (Strophe 1/2 getrennt) | Rolle zählt zusammen |
| Beat als 2-Track vs. Stems | beides; bei 2-Track wird der Beat bei Bedarf intern getrennt |
| Projekt speichern/laden | Rolle, Mikro, Stil, letzte Ergebnisse werden im Ableton-Projekt gespeichert |
| CPU | Ziel < 1 % pro Instanz; Bild wird nur gezeichnet, wenn das Fenster offen ist |
| Plugin-Absturz-Schutz | Stem-Trennung läuft außerhalb von Ableton (Abschnitt 5) |
| Bypass | Ableton-Bypass = Instanz meldet sich als inaktiv |

---

## 10. Wie wir sicherstellen, dass es stimmt

1. **Synthetische Tests:** Testsignale mit bekannten Fehlern (z. B. rosa Rauschen +4 dB bei 300 Hz, künstlicher Hall mit bekannter Nachhallzeit, absichtlich geclippte Aufnahme). Das Plugin muss genau diese Fehler mit den richtigen Zahlen melden und sonst nichts.
2. **Vorher/nachher-Test an echten Mixes:** Ein guter Mix wird absichtlich verschlechtert (EQ, zu viel Kompression). Das Plugin muss die Verschlechterung finden.
3. **Dein Ohr:** Du testest an 2–3 eigenen Songs: Sind die Hinweise richtig? Wird der Mix besser, wenn man sie umsetzt? Schwellwerte werden danach kalibriert.
4. **Gleiche Messung → gleiches Ergebnis:** automatische Tests bei jeder Code-Änderung.

---

## 11. Technik-Basis und Lizenzen (private Nutzung)

- C++ mit **JUCE** (Plugin-Gerüst, UI, Audio-Dateien). Lizenzbedingungen beim Start noch einmal prüfen, für private Nutzung unproblematisch.
- **VST3 SDK**: seit Version 3.8 (Okt. 2025) unter MIT-Lizenz.
- **ONNX Runtime** (MIT) + **HTDemucs** (MIT) für die Stem-Trennung.
- Build: Windows, Visual Studio oder automatisch über GitHub Actions.

---

## 12. Bau-Reihenfolge

1. **Mess-Kern als Test-Programm:** Man zieht einen Bounce rein und bekommt den Bericht. Kein Plugin, keine UI. So prüfen wir die Messungen und Hinweise schnell an deinen echten Songs (Abschnitt 10), bevor irgendetwas anderes gebaut wird.
2. Plugin-Hülle (VST3), Aufnahme-Check + Spur-Check mit Standard.
3. Referenz-Bibliothek mit Stem-Trennung.
4. Instanzen verbinden: Vocal gegen Beat, Master-Abgleich.
5. UI-Feinschliff.

---

## Quellen
- iZotope Tonal Balance Control, Doku "Meters and Target Curves": https://downloads.izotope.com/docs/tonal-balance-control/meters-and-target-curves/index.html
- iZotope, "Why iZotope created the Tonal Balance Control plug-in": https://www.izotope.com/en/learn/why-izotope-created-the-tonal-balance-control-plug-in.html
- Neutron 2 Masking Meter: https://s3.amazonaws.com/izotopedownloads/docs/neutron2/masking-meter/index.html · Neutron 4 Unmask: https://s3.amazonaws.com/izotopedownloads/docs/neutron4/en/unmask/index.html
- MIXROOM Manual: https://www.masteringthemix.com/pages/mixroom-manual · REFERENCE Level Line: https://www.masteringthemix.com/blogs/learn/the-most-powerful-feature-in-reference-the-level-line
- Automatische Maskierungs-Reduktion (Ronan, Ma, Reiss u. a.): https://arxiv.org/abs/1803.09960
- Blinde Nachhall-Schätzung (Maximum Likelihood): https://arxiv.org/abs/1511.04063
- HTDemucs in C++/ONNX (Mixxx, 2025): https://mixxx.org/news/2025-10-27-gsoc2025-demucs-to-onnx-dhunstack/
- VST3 MIT-Lizenz: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/VST3+License
- JUCE-Forum, Spurname in Ableton: https://forum.juce.com/t/audioprocessor-add-mechanism-to-be-able-to-distinguish-between-empty-track-name-and-not-reported-name/59428
- JUCE-Forum, Instanzen verbinden: https://forum.juce.com/t/two-instances-of-my-plugin-communicating-with-each-other-in-a-daw-project-whats-a-good-approach/62913
