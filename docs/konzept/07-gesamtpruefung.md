# NONIUS: Gesamtprüfung des Konzepts (Stand 08.11.2026)

Geprüft wurden 01–06, alle Mockups und der ganze Gesprächsverlauf. Ergebnis: **8 Widersprüche/Unklarheiten** (unten jeweils mit Entscheidung), **6 offene Punkte**, die vor oder während Phase 1 erledigt werden, und eine Einschätzung, ob gebaut werden kann.

---

## A. Widersprüche und Unklarheiten → Entscheidung

| # | Wo | Problem | Entscheidung |
|---|---|---|---|
| 1 | 01–05 | Name wechselt (Mix-Check, Echolot, Kalibr) | **NONIUS** überall |
| 2 | 02 vs. 05 | 02 nennt "Vorher/Nachher" als eigenen Modus, später gestrichen, in 05 automatisch zurück | Kein eigener Modus. Zwei Instanzen auf derselben Spur zeigen automatisch die Differenz. |
| 3 | 03 vs. 04 vs. 05 | Zentrale einmal als Master-Plugin, einmal in jeder Instanz, einmal im Hilfsprogramm | **Zentrale = Fenster im Hilfsprogramm** (anpinnbar). Zusätzlich gibt es eine **Master-Instanz** (Rolle "Summe"), weil nur sie die echte Summe hört: Sie wird für Fader-Schätzung und Erkennung stummer Spuren gebraucht. Sie hat dasselbe Fenster wie jede Spur. |
| 4 | 03 vs. 04 | 03 listet viele eingebaute Stile (Trap, Drill UK …); 04 sagt "Standard ist Normalfall" | **Keine eingebauten Stil-Profile in v1.** Ohne echte Messdaten wären sie geraten. "Richtung" = einer deiner Referenz-Ordner. |
| 5 | 04 §6 | Woher kommt "Standard" genau? Literatur liefert nur wenige Werte (Vocal +3 dB über Beat, Vocal ca. −3 LU zum Mix), aber **keine Klangkurve für Rap-Vocals** | Standard = **ein Ordner "Standard" mit 20–50 Songs**, die du gut findest, einmal eingelesen. Literaturwerte als Startwerte, solange der Ordner leer ist. → **Aufgabe für dich vor Phase 2** (siehe C). |
| 6 | Mockups vs. 04 | Was machen "Messen" und "Festhalten" genau, wenn das Plugin sowieso live misst? | **Live:** Anzeige und Befund laufen mit (über 3-s-Fenster, Hinweise erst nach stabiler Abweichung). **Messen:** wertet die letzten 8 Takte gründlich aus, inkl. aufwendigem Gehörmodell im Hilfsprogramm, Ergebnis bleibt stehen. **Festhalten:** friert die aktuelle Anzeige ein. |
| 7 | 02/06 | Mikro wird pro Instanz eingestellt, ist aber pro Spur gleich | **Mikro pro Spur:** einmal einstellen, alle Instanzen auf der Spur übernehmen es. Wirkt v. a. in Stufe 1–2. |
| 8 | Mockup N3 | Ziel-Auswahl (Standard/Ordner) ist aus dem Kopf verschwunden | Kommt in das Menü hinter "Rap Lead · NTK ▾" (Rolle, Mikro, Ziel an einer Stelle). |

---

## B. Offene Punkte

| # | Punkt | Art | Wann |
|---|---|---|---|
| 1 | **Hinweis-Katalog ausschreiben** (40–60 Checks: Messung, Grenzwert-Startwert, Text, Fix-Formel) | Spezifikation | **vor Phase 1** (nächster Schritt) |
| 2 | **Fehlende Fenster designen:** Einstellungs-Menü, Aufnahme-Ansicht (Instanz auf Rohspur sieht anders aus), Zentrale, Referenz-Bibliothek, Leer-Zustände ("zu wenig Daten", "nicht verbunden"), Fenstergröße/Skalierung (100/125/150 %) | Design | vor Phase 2 |
| 3 | **Ableton-Verhalten testen** (7 Punkte aus 05 §7) | Test an deinem PC | Phase 0 |
| 4 | **Genauigkeit:** Hall-Schätzung, Stufen-Erkennung, Fader-Schätzung | Messen an echten Daten | Phase 1 |
| 5 | **Mikro-Datenbank** füllen (Datenblätter, Kurven digitalisieren) | Datenarbeit | parallel zu Phase 2 |
| 6 | **JUCE-Lizenz** final prüfen (für private Nutzung unkritisch, für Weitergabe/Verkauf relevant) | Klärung | vor Phase 2 |

Keine dieser Punkte ändert die Architektur. Recherche-Lücken, die das Bauen blockieren, gibt es nicht mehr. Was offen ist, lässt sich nur mit echten Daten klären.

---

## C. Was ich von dir brauche (später)
1. Einmal 15 Min. Ableton-Test mit einem Test-Plugin (Phase 0).
2. 2–3 eigene Songs als Bounces: Rohspur Vocal, fertige Vocal-Spur, Beat (Phase 1).
3. 20–50 Referenz-Songs für "Standard" in einen Ordner (vor Phase 2).
4. Ein leeres GitHub-Repo, wenn wir starten.

---

## D. Bau-Plan (aktualisiert)
- **Phase 0 (klein):** Test-Plugin, das nur Abletons Verhalten mitschreibt (05 §7). Ergebnis entscheidet die Detail-Fragen.
- **Phase 1:** Mess-Kern + Hinweis-Katalog als Test-Programm: Bounce reinziehen → Bericht. Kalibrieren mit synthetischen Tests und deinen Songs.
- **Phase 2:** VST3-Plugin mit dem N3-Fenster, Stufen, Aufnahme-Check, Standard-Profil.
- **Phase 3:** Hilfsprogramm: Referenz-Bibliothek mit Stem-Trennung, Zentrale.
- **Phase 4:** Instanzen verbinden: Vocal gegen Beat, Master-Abgleich, Differenz zwischen zwei Instanzen.
- **Phase 5:** Mikro-Datenbank komplett, UI-Feinschliff.

## E. Einschätzung
**Ja, bereit zum Bauen**, mit einer Bedingung: Zuerst den Hinweis-Katalog ausschreiben (B1), weil er festlegt, was Phase 1 messen muss. Das ist Schreibarbeit, keine Recherche. Danach kann Phase 0/1 starten. Die übrigen offenen Punkte entstehen ganz natürlich während der Phasen und blockieren den Start nicht.
