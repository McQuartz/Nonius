# KALIBR: Mikrofon-Profile (Stand 08.11.2026)

## Wofür das Mikro überhaupt wichtig ist
Das Ziel (wie das Vocal am Ende klingen soll) hängt **nicht** vom Mikro ab. Das Mikro hilft bei zwei Dingen:
1. **Ursachen erkennen:** Ist das Vocal hell, weil das Mikro hell ist (z. B. Präsenz-Anhebung), oder weil es falsch bearbeitet wurde? Ist der Raum hörbar, weil das Mikro eine Kugel-Charakteristik hat?
2. **Aufnahme-Hinweise:** Wie viel Gain braucht das Mikro? Ist das Rauschen normal für dieses Mikro? Ist der Bass-Anstieg normaler Nahbesprechungseffekt?

Wenn das Mikro unbekannt ist, funktioniert die Analyse trotzdem. Nur die Hinweise zur Ursache werden allgemeiner.

## Welche Werte pro Mikro gespeichert werden
| Wert | Wofür |
|---|---|
| Bauart: dynamisch / Kondensator (FET) / Röhre / Bändchen / USB / Modeling | Rauschen, Raumempfindlichkeit, Impulsverhalten |
| Richtcharakteristik(en), umschaltbar | Wie viel Raum aufgenommen wird (Niere < Superniere < Breite Niere < Kugel); Nahbesprechungseffekt (bei Kugel fast keiner) |
| Empfindlichkeit (mV/Pa) | Erwarteter Gain am Interface (z. B. SM7B braucht sehr viel), ob Rauschen vom Preamp kommt |
| Eigenrauschen (dB-A) | Ob Rauschen in Pausen normal ist |
| Max. SPL, Pad | Ob Verzerrung vom Mikro kommen kann |
| Frequenzgang als Kurve (aus Hersteller-Diagramm digitalisiert) | Typische Färbung: Präsenz-Anhebung, Bass-Abfall, Höhen |
| Schalter: Low Cut, Präsenz-Boost usw. | Welche Stellung benutzt wurde (im Plugin auswählbar) |
| Nahbesprechung (Stärke, typischer Abstand) | Bass-Wummern einordnen |

Bei Modeling-Mikros (Slate ML-1, UA Sphere, Antelope Edge) wird zusätzlich gespeichert, dass der Klang vom gewählten Modell abhängt. Dann wählt man im Plugin das Modell.

## Wie die Liste entsteht
- **Quelle:** offizielle Datenblätter (PDF) der Hersteller. Erster Test-Durchlauf hat gezeigt: Die normalen Produktseiten enthalten oft keine Zahlen (z. B. Shure, Neumann teilweise), die Datenblätter schon. Werte, die nicht im Datenblatt stehen, bleiben leer. Es wird nichts geschätzt.
- **Frequenzgang-Kurven:** werden aus den Hersteller-Diagrammen digitalisiert (ca. 30–60 Punkte pro Kurve). Das ist Handarbeit pro Mikro und wird beim Bauen als eigener Daten-Schritt erledigt.
- **Umfang Version 1:** ca. 60–80 der verbreitetsten Vocal-Mikros (Home-Studio bis Profi-Studio). Startliste unten.
- **Unbekanntes Mikro:** Auswahl "anderes" + Bauart + Richtcharakteristik. Dann gelten allgemeine Werte für diese Bauart.
- **Erweiterbar:** Die Liste ist eine Datei, neue Mikros kommen per Update dazu. Man kann auch selbst eins eintragen.

## Startliste (Auszug, wird erweitert)
Shure SM7B, SM7dB, SM58, Beta 58A, Beta 87A, KSM32, MV7+ · Rode NTK, NT1 (5. Gen), NT1-A, NT2-A, K2, Procaster, PodMic · Neumann TLM 102, TLM 103, U 87 Ai, TLM 107, U 47 fet, KMS 105 · AKG C214, C414 XLII/XLS, C12 VR · Audio-Technica AT2020, AT2035, AT4040, AT4050 · Electro-Voice RE20, RE320 · Warm Audio WA-87 R2, WA-47 · Sony C-800G · Lewitt LCT 440 Pure, LCT 640 TS · Aston Origin, Spirit · Mojave MA-201fet · Manley Reference Cardioid · sE Electronics X1 S, sE2200 · Blue Yeti, Bluebird SL · Telefunken TF51 · Austrian Audio OC18 · Modeling: Slate VMS ML-1, UA Sphere LX, Antelope Edge Solo

## Bereits erfasste Beispiele (aus Hersteller-Seiten)
| Mikro | Bauart | Richtung | Empf. | Rauschen | Max SPL | Schalter | Quelle |
|---|---|---|---|---|---|---|---|
| Lewitt LCT 440 Pure | Kondensator, Großmembran 1" | Niere | 27,4 mV/Pa | 7 dB(A) | 140 dB | – | lewitt-audio.com |
| Lewitt LCT 640 TS | Kondensator (FET), Großmembran, Dual Output | Kugel, breite Niere, Niere, Superniere, Acht | 31,4 mV/Pa | 10 dB(A) | 134 dB | Pad 0/−6/−12/−18, Low Cut 40/80/160 Hz | lewitt-audio.com |
| Neumann TLM 103 | Kondensator | Niere | 23 mV/Pa | (Datenblatt) | 138 dB | – | neumann.com |
| Shure SM7dB | dynamisch mit eingebautem Preamp | Niere | (Datenblatt) | – | – | Preamp +18/+28 dB, Bypass, Bass-Roll-off, Präsenz | shure.com |
