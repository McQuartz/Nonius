<!-- Erzeugt mit tools/calibrate (nonius-kalibrieren), Basis: trockene Vocal-Spur von Ace, Raumantworten aus pyroomacoustics 0.10.1 -->
# NONIUS Kalibrierung mit künstlichen Fehlern

Basis: `boomversedry.mp3`, 15–105 s, Low Cut 120 Hz, Peak −8 dBFS. In diese Spur werden Fehler mit bekannter Größe eingebaut.

## Popps (A07)

Künstlicher Popp: Tiefpass-Stoß 30–80 Hz, 40 ms, an Wortanfängen. Gleichzeitig zählt, was ohne eingebaute Popps gemeldet wird (Fehlalarme).

| Popp-Pegel (relativ zum Wort-Peak) | eingebaut | gefunden | verpasst | Fehlalarme |
|---|---|---|---|---|
| keine (Basis) | 0 | 0 | 0 | 0 |
| 0 dB | 11 | 11 | 0 | 0 |
| -6 dB | 11 | 10 | 1 | 0 |
| -12 dB | 11 | 11 | 0 | 0 |

## Clipping (A01)

Spur um x dB verstärkt und bei 0 dBFS hart begrenzt, danach wieder auf −8 dBFS abgesenkt (wie beim Bearbeiten einer übersteuerten Aufnahme).

| Übersteuerung | Samples über 0 dBFS | gefundene Stellen |
|---|---|---|
| keine | 0 | 0 |
| +1 dB | 41 | 4 |
| +3 dB | 625 | 50 |
| +6 dB | 9299 | 219 |

## Rauschen (A04)

Weißes Rauschen mit bekanntem Pegel addiert (A-bewertet gemessen).

| Rauschen (unbewertet RMS) | gemessen (A) | Status |
|---|---|---|
| -80 dBFS | -71.8 dBFS | OK |
| -65 dBFS | -66.2 dBFS | OK |
| -55 dBFS | -57.2 dBFS | ACHTUNG |
| -45 dBFS | -47.6 dBFS | PROBLEM |

## Brummen (A05)

50 Hz + Obertöne (100, 150 Hz, je −6 dB) plus Rauschen −70 dBFS als Pausen-Untergrund.

| Brumm-Pegel | gemessen über Nachbarn | erkannt |
|---|---|---|
| keins | 5.6 dB bei 50 Hz | nein |
| -75 dBFS | 10.6 dB bei 50 Hz | ja |
| -65 dBFS | 13.9 dB bei 50 Hz | ja |
| -55 dBFS | 17.2 dB bei 50 Hz | ja |

## Raumhall (A06)

Gefaltet mit künstlicher Raumantwort (exponentiell abklingendes Rauschen, bekannte RT60), 50 % Direktschall.

| RT60 eingebaut | geschätzt | Sicherheit | Ausklänge |
|---|---|---|---|
| keiner (Basis) | 0.21 s | 1.0 | 152 |
| 0.3 s | 0.33 s | 1.0 | 331 |
| 0.6 s | 0.48 s | 1.0 | 312 |
| 1.0 s | 0.63 s | 1.0 | 206 |

## Raumhall in simulierten Räumen (A06)

Raumantworten aus dem Raum-Simulator pyroomacoustics (Spiegelquellen), Mikro nah an der Quelle wie bei einer Vocal-Aufnahme.

| Raum | Mikro-Abstand | RT60 (Simulator) | RT geschätzt | Raumanteil (60–120 ms nach Wortende) | Wortenden |
|---|---|---|---|---|---|
| kabine_0.25s | 15 cm | 0.24 s | 0.25 s (umgerechnet 0.25 s) | -17.5 dB | 111 |
| zimmer_0.5s_nah | 15 cm | 0.51 s | 0.28 s (umgerechnet 0.35 s) | -16.3 dB | 114 |
| zimmer_0.5s_30cm | 30 cm | 0.52 s | 0.41 s (umgerechnet 0.77 s) | -14.5 dB | 34 |
| hallig_0.8s_nah | 15 cm | 0.85 s | 0.38 s (umgerechnet 0.65 s) | -14.1 dB | 46 |
| hallig_0.8s_30cm | 30 cm | 0.87 s | 0.49 s (umgerechnet 1.03 s) | -150.0 dB | 7 |
| sehr_hallig_1.2s | 20 cm | 1.34 s | 0.58 s (umgerechnet 1.31 s) | -150.0 dB | 1 |

## Zischlaute (T06)

High Shelf ab 5 kHz mit bekanntem Gain. Erwartung: Messwert steigt um ungefähr den Gain.

| High Shelf | S-Laute relativ zu Vokalen | Änderung | Anzahl S-Laute | Status |
|---|---|---|---|---|
| +0 dB | -3.7 dB | – dB | 135 | OK |
| +3 dB | -1.8 dB | +2.0 dB | 162 | OK |
| +6 dB | 0.3 dB | +4.0 dB | 185 | ACHTUNG |
| +9 dB | 2.3 dB | +6.1 dB | 213 | PROBLEM |


## Zusammenfassung (Stand 08.11.2026)

| Check | Ergebnis | Folge |
|---|---|---|
| A07 Popps | 32 von 33 eingebauten Popps gefunden, 0 Fehlalarme | zuverlässig |
| A01 Clipping | ab 41 übersteuerten Samples erkannt, keine Fehlalarme | zuverlässig (WAV; bei MP3 eingeschränkt) |
| A04 Rauschen | Fehler 1–3 dB ab −65 dBFS; darunter begrenzt die Stimme selbst die Messung (dichter Rap ohne Pausen) | zuverlässig im relevanten Bereich |
| A05 Brummen | Grundwert ohne Brummen 5,6 dB, erkannt ab ca. −75 dBFS | Grenze 10 dB |
| A06 Raumhall | Rohwert steigt mit dem Raum, unterschätzt aber bei nahem Mikro deutlich (0,85 s → 0,38 s). Umrechnung ±50 % | Bewertung mit Rohwert: > 0,32 s ACHTUNG, > 0,45 s PROBLEM; bleibt experimentell |
| T06 Zischlaute | Messwert folgt einem High Shelf zu ca. 70 % (Shelf +9 dB → +6,1 dB) | Grenzen: > −1 dB ACHTUNG, > +2 dB PROBLEM, < −14 dB gelispelt |

Offen: Kalibrierung an echten Profi-Spuren (Cambridge Multitracks, MUSDB18), sobald der Zugriff da ist.
