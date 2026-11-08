# Mix-Check: Vertiefung & Änderungen am Konzept (Stand 08.11.2026)

Zweite Recherche-Runde. Ändert bzw. präzisiert 04-technik-konzept.md. Wo sich etwas ändert, steht **Änderung**.

---

## 1. Stufen-Modell (neu)

Das Plugin teilt nicht nach Geräten ein, sondern nach **Zielen**. So ist die Reihenfolge deiner Kette egal.

| Stufe | Ziel | Geprüft wird (Auszug) |
|---|---|---|
| 1 Aufnahme | Rohmaterial brauchbar | Pegel, Clipping, Raumhall, Rauschen/Brummen, Popps, DC |
| 2 Sauber | Störendes raus | Rumpeln < 80 Hz, Mumpf, stehende Resonanzen, Popps, Atmer zu laut |
| 3 Kontrolliert | Dynamik im Zielbereich | Crest-Factor, Short-term-Streuung, Wortspitzen, Pumpen |
| 4 Ton & Charakter | klingt wie gewollt | Tonalität gegen Referenz, Zischlaute, Härte (auch nach Sättigung), Air |
| 5 Im Mix | sitzt im Beat | Lautstärke gegen Beat, Maskierung, Stereo/Mono |

- Jede Instanz zeigt einen **Fortschritt** über die Stufen: welche Ziele schon erreicht sind und welche nicht.
- Die Stufe, an der eine Instanz steht, wird automatisch geschätzt (z. B. Rohspur = viel Dynamik, Raum, Rumpeln) und kann von Hand gesetzt werden.
- Referenz-Vergleich nur in Stufe 4–5 bzw. am Ende der Kette. In früheren Stufen wird nur geprüft, was dort schon stimmen sollte. Sonst bekäme man "zu wenig komprimiert" angezeigt, bevor man überhaupt komprimiert hat.
- Zwei Instanzen auf derselben Spur → automatisch "Was hat sich dazwischen verändert" (Differenz der Messungen). Das ist das frühere Vorher/Nachher, ohne dass man es extra einrichten muss. Erkennen, welche Instanz vorne sitzt: Die hintere hat die Veränderungen; zusätzlich zeitliche Verzögerung durch Geräte dazwischen. Muss in Ableton getestet werden (siehe 7).

---

## 2. Zentrale als eigenes Fenster (neu)

- Laut Ableton-Handbuch (Live 12, Kap. 24.1.1) zeigt "Plug-In-Fenster automatisch ausblenden" nur die Fenster der *ausgewählten* Spur. Deshalb verschwindet dein Master-Fenster beim Spurwechsel.
- **Lösung:** Die Zentrale läuft im Hilfsprogramm "Mix-Check Library" (das sowieso die Referenzen analysiert). Das ist ein normales Windows-Fenster, kein Plugin-Fenster. Ableton blendet es nicht aus. Es kann auf einen zweiten Bildschirm, "immer im Vordergrund" ist einstellbar.
- Knopf im Plugin: "Zentrale öffnen". Das Hilfsprogramm startet, falls nicht schon offen.
- **Änderung zur Technik:** Weil die Zentrale in einem eigenen Programm läuft, reicht gemeinsamer Speicher innerhalb von Ableton nicht mehr. Die Instanzen teilen ihre Messwerte deshalb über **Shared Memory des Betriebssystems** (Memory-Mapped File) mit dem Hilfsprogramm. Nur Messwerte, kein Audio, ca. 10-mal pro Sekunde. Innerhalb von Ableton bleiben die Instanzen zusätzlich direkt verbunden, damit alles auch ohne Hilfsprogramm funktioniert.
- Alternative, verworfen: Das Plugin selbst öffnet ein zweites Fenster. Möglich, aber in Ableton anfällig (Fokus, Tastatur, Abstürze beim Schließen des Projekts). Das eigene Programm ist robuster.

---

## 3. Maskierung (Vocal gegen Beat): Gehörmodell festgelegt

Zwei bekannte Verfahren aus der Forschung:

| Verfahren | Wie | Bewertung |
|---|---|---|
| Teil-Lautheit nach Glasberg/Moore (Ward, Reiss & Athwal 2012; Prinzip auch bei Neutron) | Außen-/Mittelohr-Filter, Filterbank, Erregungsmuster; Maskierung = Lautheit allein minus Lautheit im Mix (Phon). | Am genauesten, aber sehr rechenintensiv; die Autoren nutzen es offline. |
| Mithörschwelle wie bei MPEG-Codecs (Ronan et al. 2018) | Pro Bark-Band: Mithörschwelle des Beats berechnen; liegt das Vocal darunter, ist es verdeckt. Maß = Summe der Unterschreitung in dB, gedeckelt bei 20 dB. | Leicht genug für Echtzeit, gut erprobt. |

**Festlegung:** Live (in Ableton) die MPEG-artige Mithörschwelle in ca. 25 Bark-Bändern, gemittelt über 3-s-Fenster, gezählt wie bei Neutron (Anteil der Zeit, in der ein Band verdeckt ist). Für den genauen Bericht ("8 Takte prüfen") zusätzlich die Glasberg/Moore-Teil-Lautheit, berechnet im Hilfsprogramm, wo Rechenzeit keine Rolle spielt.

Fix-Vorschlag bei Maskierung: welches Band, wie viel dB beim Beat absenken (bevorzugt dynamisch, nur wenn das Vocal da ist) oder beim Vocal anheben. Grenze wie in der Forschung: höchstens ±6 dB pro Band, die Lautheit des Beats darf dadurch nicht zu stark sinken.

---

## 4. Startwerte aus der Forschung

Bevor deine Referenzen da sind, braucht der Standard Startwerte. Gefunden:

| Messung | Wert | Quelle |
|---|---|---|
| Lead-Vocal lauter als Begleitung (Rap) | im Mittel **+3,2 dB** (Bereich 2,7–3,6), A-bewertet, 1-s-Fenster | Gerdes & Siedenburg 2023, 414 Songs, Grammy-Kategorien |
| zum Vergleich | Pop +2,7 dB, Rock +0,2 dB; Solo-Künstler höher als Bands | dieselbe Studie |
| Lead-Vocal relativ zum ganzen Mix | ca. −3 LU (±1,6) | De Man et al. 2014 (kleine Stichprobe) |
| Lead-Vocal mittig | Seite/Mitte ≈ 0,02 | De Man et al. 2014 |
| Spektraler Schwerpunkt Lead-Vocal | ca. 2,9 kHz (Mix ca. 2,5 kHz) | De Man et al. 2014 (Rock/Pop, nur grobe Orientierung) |

**Änderung:** Das Vocal-zu-Beat-Verhältnis wird genau wie in der Gerdes-Studie gemessen (A-bewertet, 1-s-Fenster, leise Teile verworfen). Dann sind unsere Werte direkt mit der Studie vergleichbar, und der Startwert +3 dB für Rap ist belastbar.

Die Gerdes-Studie hat ihre Messung auch geprüft: Mit getrennten Stems weicht das Ergebnis im Mittel nur 0,7 dB vom echten Wert ab (ohne Backing-Vocals). Mit Backing-Vocals sind es 1,6 dB. Das zeigt: Stem-Trennung für Referenzen funktioniert für das Lautstärkeverhältnis gut genug, Backing-Vocals/Adlibs machen es ungenauer.

---

## 5. Stem-Trennung: Modellwahl

| Modell | Vocal-Qualität (SDR, MUSDB18) | Zeit pro Song | Bemerkung |
|---|---|---|---|
| Spleeter | ca. 5,9 dB | Sekunden | zu ungenau |
| HTDemucs | ca. 8 dB | ca. 1 Min. (CPU) | MIT, läuft nachweislich in C++ (Mixxx) |
| HTDemucs fine-tuned | ca. 9 dB | 3–4× langsamer | MIT |
| BS-RoFormer / Mel-RoFormer | höher (Ø ca. 9,8 dB) | ähnlich wie ft | Lizenz der jeweiligen Gewichte prüfen |

Zahlen teils aus Hersteller-Benchmarks, deshalb nur grob. **Festlegung:** Start mit HTDemucs (sicher lauffähig, klare Lizenz). Austauschbar gebaut, damit später ein besseres Modell eingesetzt werden kann. Für dich kein großer Unterschied: Referenzen werden einmal analysiert, Wartezeit pro Song ca. 1–3 Min.

---

## 6. Messmethoden präzisiert

**Kompression/Dynamik**
- Crest-Factor auf zwei Zeitskalen (100 ms und 1 s), wie bei De Man. Kurze Skala = Transienten/Wortspitzen, lange = Phrasen-Dynamik.
- Streuung der Short-term-Lautheit, Verteilung der Pegel (wie "breit" sie ist).
- Erkenntnis aus der Forschung: Gut gemischte Spuren haben nach der Bearbeitung oft sogar *höhere* Crest-Faktoren, weil Kompressoren mit langsamer Attack die Transienten durchlassen. Also nicht "je weniger Dynamik desto besser", sondern Bereich aus Referenzen.
- Pumpen: Pegel des Hintergrunds (Raum, Atmer) steigt in Pausen schnell an → Release zu kurz / zu viel Gain Reduction.
- ML-Erkennung von Kompressor-Einstellungen gibt es in der Forschung (95 %+ bei groben Klassen), ist für v1 aber nicht nötig. Später möglich.

**Sättigung/Distortion**
- Ehrlich: Am fertigen Signal lässt sich Sättigung nicht sicher messen, man weiß nicht, wie es ohne klang.
- Deshalb: (a) Mit zwei Instanzen (vor/nach der Distortion) wird die Veränderung exakt gemessen: zusätzliche Obertöne, Anstieg 2–5 kHz, Verlust an Crest-Factor. (b) Mit einer Instanz nur indirekt über Härte (2–5 kHz gegen Ziel) und Rauheit (schnelle Pegelschwankungen im Hochton).

**Zischlaute**
- S-Ereignisse über Energie-Schwerpunkt > 4–5 kHz und kurze Dauer erkennen, Pegel relativ zu den Vokalen messen, Verteilung über alle S-Laute. Bei Referenz-Vocals genauso.

**Resonanzen**
- Feines Spektrum (1/24 Oktave) minus geglättetes (1/3 Oktave). Spitzen, die über mindestens 70 % der aktiven Zeit stehen bleiben, sind Resonanzen (Raum, Mikro); kurz auftauchende Spitzen sind Tonhöhe der Stimme und werden ignoriert.

---

## 7. Was nur an deinem Rechner getestet werden kann

Dafür baue ich später ein winziges Test-Plugin, das nur Daten mitschreibt. Ca. 15 Min. Aufwand für dich.

1. Verschwindet das Master-Plugin-Fenster genau so, wie du es kennst? (Handbuch sagt: nur ausgewählte Spur sichtbar.)
2. Laufen Plugins auf stummen oder deaktivierten Spuren weiter?
3. Laufen Plugins weiter, wenn der Transport steht?
4. Meldet Ableton allen Spuren dieselbe Zeitposition (inkl. Latenzausgleich)?
5. Kommt der Spurname in Live 12 richtig an?
6. Laufen alle Instanzen im selben Prozess?
7. Kann das Hilfsprogramm per Shared Memory mit den Instanzen reden (Windows-Rechte, Virenscanner)?

Diese Antworten ändern nichts an der Grundarchitektur, nur an Details. Für jede gibt es einen Plan B.

---

## 8. Was jetzt feststeht vs. was offen ist

**Fest:** Analyse-Plugin ohne Klangveränderung · Echtzeit-Teil minimal · Stufen-Modell nach Zielen · Standard als Normalfall, Richtung optional, eigene Referenz-Ordner · Stem-Trennung für Referenzen im Hilfsprogramm · Zentrale als eigenes Fenster · Maskierung nach Mithörschwelle (live) und Teil-Lautheit (Bericht) · fester Hinweis-Katalog · Start mit Test-Programm für Bounces.

**Offen / wird mit Tests geklärt:** genaue Schwellwerte (Kalibrierung an deinen Songs) · Genauigkeit der Hall-Schätzung · Stufen-Erkennung · Punkte aus Abschnitt 7.

---

## Quellen
- Ableton Live 12 Handbuch, Plug-in-Fenster: https://www.ableton.com/en/live-manual/12/using-plug-ins/
- Ward, Reiss, Athwal (AES 133, 2012), Mixing mit Lautheit/Teil-Lautheit: https://joshreiss.github.io/documents/2012/WardReissAthwal-AES133-Multitrackmixingusingamodelofloudnessandpartialloudness.pdf
- Ronan et al. (2018), Maskierung minimieren: https://arxiv.org/abs/1803.09960
- De Man et al. (ISMIR 2014), Analyse echter Mixes: https://archives.ismir.net/ismir2014/paper/000293.pdf
- Gerdes & Siedenburg (JASA-EL 2023), Lead-Vocal-Lautstärke 1946–2020: https://www.geidai.ac.jp/~marui/course_enshu_2024/gerdes_siedenburg_202304.pdf
- Kompressor-Erkennung (Sun et al. 2024): https://arxiv.org/abs/2411.04337
- Benchmark Stem-Trennung (Hersteller-Angaben, nur grob): https://dev.to/codesugar_lin_037a57b06a4/htdemucs-vs-bs-roformer-vs-spleeter-a-2026-audio-source-separation-benchmark-2ll8
- HTDemucs in C++ (Mixxx): https://mixxx.org/news/2025-10-27-gsoc2025-demucs-to-onnx-dhunstack/
