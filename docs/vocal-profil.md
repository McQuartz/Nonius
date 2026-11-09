# NONIUS Vocal-Profil aus deinen Referenzen

Stand 09.11.2026. Grundlage: deine 29 Referenz-Songs, mit HTDemucs (htdemucs, `--two-stems=vocals`) in Vocal und Beat getrennt, danach mit dem NONIUS-Messkern gemessen (`tools/vocal`). Ins Profil gehen 28 Songs ein. Favoriten zählen doppelt (The Stickup, GO!, Wu Tang Forever, Sonderfahrt). "Got It On Me" wurde nur zum Vergleich gemessen.

Die Datei `plugin/Resources/vocal-profil.json` hat dasselbe Format wie `standard-profil.json`. Das Plugin nutzt sie für beide Vocal-Rollen:

- **Vocal bearbeitet:** Ein Befund kommt nur, wenn ein Wert außerhalb von P10–P90 liegt.
- **Vocal roh:** Gezeigt wird der Weg vom Rohwert zum Median, also wie viel Kompression, welcher EQ und welcher De-Esser nötig sind.

## Wie genau sind getrennte Vocals? (MUSDB18-Prüfung)

Für die Prüfung habe ich 14 Songs aus dem MUSDB18-Testset genommen, jeweils einen Ausschnitt von 90 s. Diese Songs hat HTDemucs beim Training nicht gesehen, und zu jedem gibt es die echte Vocal-Spur. Verglichen wurde der HTDemucs-Stem mit der echten Spur, Angabe als Median, in Klammern P10 bis P90.

| Messwert | Fehler der Trennung | Urteil |
|---|---|---|
| Silben-Dynamik | +0,05 dB (−0,2 bis +0,4) | sehr genau |
| Wort-Spitzen | +0,1 LU (−0,1 bis +0,2) | sehr genau |
| Crest 100 ms / 1 s | ±0,1 dB | sehr genau |
| Zischlaute (S zu Vokal) | 0,0 dB (−0,9 bis +0,6) | genau |
| Klangkurve 125 Hz – 16 kHz | unter 1 dB, 125 Hz −0,9 dB | genau |
| Vocal über Beat | +0,4 dB (0,0 bis +1,0) | genau, Median korrigiert |
| Abstand Wort zu Lücke | +5,5 dB (+1,8 bis +17,6) | **grob**: HTDemucs schneidet Hallfahnen in den Lücken mit ab. Median korrigiert, der Bereich bleibt breit |
| Nachklang nach Wortende | −2,2 dB | korrigiert |
| Oktaven 31 / 63 Hz | −5 dB, Streuung ±5 dB | **unbrauchbar**, nicht im Profil. Hier gilt der feste Low-Cut-Check |

Die Korrekturen stehen in `tools/vocal/daten/korrektur.json`, die vollständige Prüfung in `tools/vocal/daten/pruefung-musdb18.json`.

## Zielbereiche "Vocal bearbeitet" (P10 / Median / P90)

| Messwert | P10 | Median | P90 | Boom Bap wet | Drill wet |
|---|---|---|---|---|---|
| Silben-Dynamik (dB) | 6,0 | 7,9 | 9,7 | 5,3 ↓ | 3,6 ↓ |
| Wort-Spitzen (LU) | 1,8 | 2,5 | 3,4 | 1,4 ↓ | 1,2 ↓ |
| Crest 1 s (dB) | 13,0 | 14,2 | 15,0 | 12,2 ↓ | 15,2 |
| Zischlaute S zu Vokal (dB) | −4,8 | −2,3 | +2,2 | −0,5 | +2,2 |
| Abstand Wort zu Lücke (dB, grob) | 15,5 | 20,7 | 37,7 | 17,6 | 15,3 ↓ |
| Spektrale Neigung (dB/Okt.) | −0,8 | −0,2 | +0,7 | −1,0 ↓ | −0,5 |
| Vocal über Beat (dB, A-bewertet) | 2,4 | 3,4 | 5,4 | 2,9 | – |

Die Klangkurve ist in dB relativ zur Gesamtleistung angegeben, also unabhängig von der Lautstärke.

| Oktave | 125 | 250 | 500 | 1k | 2k | 4k | 8k | 16k |
|---|---|---|---|---|---|---|---|---|
| Referenz P10 | −24,5 | −9,6 | −6,7 | −8,7 | −11,3 | −15,3 | −16,2 | −24,8 |
| Referenz Median | −18,3 | −7,4 | −5,0 | −6,8 | −8,7 | −13,7 | −13,5 | −19,0 |
| Referenz P90 | −15,0 | −5,5 | −4,0 | −5,1 | −7,0 | −10,9 | −10,7 | −13,4 |
| Boom Bap dry | −8,4 ↑ | −7,6 | −3,0 ↑ | −10,4 ↓ | −15,4 ↓ | −17,3 ↓ | −15,9 | −22,7 |
| Boom Bap wet | −8,7 ↑ | −7,6 | −4,2 | −9,1 ↓ | −13,5 ↓ | −13,3 | −10,9 | −17,0 |
| Drill dry | −7,9 ↑ | −8,2 | −3,8 | −10,0 ↓ | −13,6 ↓ | −16,4 ↓ | −12,8 | −21,1 |
| Drill wet | −7,1 ↑ | −9,0 | −6,2 | −9,9 ↓ | −10,7 | −11,3 | −8,5 ↑ | −15,3 |

## Was NONIUS damit zu deinen Spuren sagt

**Boom Bap dry (Rolle roh):** Kompressor ca. 2 dB GR (Ratio 4:1, Attack 5–10 ms). EQ −6 dB bei 125 Hz, +5 dB bei 2 kHz, +2,5 dB bei 1 kHz und 4 kHz. Low Cut 80–100 Hz.

**Boom Bap wet (Rolle bearbeitet):** Überkomprimiert, Gain Reduction ca. 3 dB verringern oder die Attack langsamer stellen. Immer noch ca. 10 dB zu viel um 125 Hz, 2 kHz fehlt ca. 5 dB.

**Drill wet:** Stark überkomprimiert (Silben-Dynamik 3,6 statt 7,9 dB). 125 Hz deutlich zu viel, 8 kHz ca. 5 dB zu viel, 1 kHz zu wenig. Die Lücken sind etwas voller als bei den Referenzen.

Das auffälligste Ergebnis: Deine Referenzen lassen den Wörtern mehr Kontrast, als man erwarten würde. Pro-Vocals schwanken von Silbe zu Silbe um ca. 8 dB, deine fertigen Vocals um 4–5 dB. Der "Abstand" zu Webby liegt also nicht an zu wenig Kompression, sondern eher an zu viel. Dazu kommen deutlich zu viel 90–180 Hz und zu wenig Präsenz bei 1–2 kHz.

## Grenzen

- **Kompression in dB ist eine Schätzung.** Grundlage ist ein Kompressor-Sweep auf deinen beiden Rohspuren (Ratio 4:1, Attack 1/5/20 ms). Die ersten ca. 3 dB Gain Reduction senken die Silben-Dynamik ungefähr 1:1. Danach entscheiden vor allem Ratio und Attack, nicht die dB-Zahl. Deshalb rät NONIUS bei großem Abstand zu zwei Stufen.
- **Die Referenz-Vocals sind gemastert.** Der Limiter des Masters steckt mit drin. Vor dem Master dürfen deine Vocals also eher etwas mehr Dynamik haben, nicht weniger.
- **Rohspur:** Mikrofon, Raum und Stimme verschieben die Klangkurve. Die EQ-Vorschläge zeigen die Richtung, sie sind kein Rezept.
- **Abstand Wort zu Lücke** bleibt grob (siehe Prüfung). Dieser Befund ist nur ein Hinweis, kein Problem.
- **Nicht geprüft:** Cambridge-Multitracks. Die Seite blockt automatische Downloads mit einer Cloudflare-Abfrage.

## Neu bauen

```
python tools/vocal/vocal_profil.py trennen --ordner "Reference Mixing" --stems stems
python tools/vocal/vocal_profil.py messen  --stems stems --tool nonius-vocal --werte ref.jsonl
python tools/vocal/vocal_profil.py profil  --werte ref.jsonl --favoriten "Stickup;GO!;Wu Tang Forever;Sonderfahrt" \
       --nur-vergleich "Got_It_On_Me" --korrektur tools/vocal/daten/korrektur.json --profil vocal-profil.json
```

Eine einzelne Spur prüfen: `nonius-vocal --vocal spur.wav --profil vocal-profil.json --roh 1`
