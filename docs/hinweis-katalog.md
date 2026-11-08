# NONIUS: Hinweis-Katalog v0.1

Stand 08.11.2026. Legt fest, **was gemessen wird, ab wann ein Hinweis kommt und was er sagt.** Alle Grenzwerte sind **Startwerte**. Sie werden in Phase 1 mit Testsignalen und echten Songs kalibriert.

## 1. Allgemeine Regeln (gelten für jeden Check)

**Ergebnis-Stufen:** `OK` · `ACHTUNG` · `PROBLEM`. Dazu kommt `KEINE DATEN`, wenn die Sicherheit zu gering ist.

**Wann ein Hinweis erscheint** (alle Bedingungen müssen erfüllt sein):
1. Der Wert liegt außerhalb des Zielbereichs (bei Ziel-Verteilungen: außerhalb P10–P90).
2. Die Abweichung ist größer als die Mindestabweichung des Checks.
3. Sie ist in mindestens 3 aufeinanderfolgenden 3-s-Fenstern stabil (Hysterese: verschwindet erst, wenn sie 3 Fenster lang wieder im Bereich ist).
4. Es gibt genug aktives Material: mindestens 8 s Gesang bzw. 8 s Signal bei Instrumenten.

**Priorität** (für die Top 3–5):
`Priorität = Abweichung in dB × Gehörgewicht(f) × Sicherheit × Stufenfaktor`
- Gehörgewicht: 1,0 bei 1–5 kHz, 0,8 bei 500 Hz–1 kHz und 5–10 kHz, 0,6 bei 100–500 Hz und über 10 kHz, 0,4 unter 100 Hz (grob nach Kurven gleicher Lautheit, wird kalibriert).
- Stufenfaktor: Checks aus *früheren* Stufen als der Position des Plugins × 1,2, weil man sie zuerst beheben sollte. Checks aus *späteren* Stufen werden nicht angezeigt.
- Sicherheit 0–1: aus Datenmenge, Stabilität und, bei Referenzen, aus der Trennqualität.

**Zusammenfassen:** Haben mehrere Hinweise dieselbe Ursache, erscheint ein gemeinsamer Hinweis. Beispiel: T03 Mumpf + T08 wenig Air → T01 "insgesamt zu dunkel".

**Fix-Formel für Klang-Checks (EQ):**
- Überschuss e(f) = Messwert − Bereichsgrenze (nur wo außerhalb), auf 1/6-Oktav-Bändern.
- Zusammenhängender Bereich f1…f2 mit e ≥ Mindestabweichung.
- Mittenfrequenz fc = mit e gewichteter geometrischer Mittelwert, auf "schöne" Werte gerundet (z. B. 320 Hz).
- Q = fc / (f2 − f1), begrenzt auf 0,5–8.
- Gain = −0,7 × max(e), begrenzt auf 1–6 dB, auf 0,5 dB gerundet.
- **Statisch oder dynamisch:** Liegt der Überschuss in < 60 % der aktiven Zeit vor → dynamischer EQ mit Threshold so, dass er nur in diesen Momenten greift. Bei ≥ 60 % → normaler EQ.

**Rollen:** LV = Lead Vocal, DB = Double, AD = Adlib, BV = Backing, BT = Beat (2-Track), KI = Kick, SN = Snare, 808 = 808/Bass, BUS = Gruppe, SUM = Master.

---

## 2. Stufe 1: Aufnahme

| ID | Check | Messung | Startwert | Rollen | Text | Fix |
|---|---|---|---|---|---|---|
| A01 | Clipping | ≥ 3 Samples in Folge an derselben Grenze nahe Vollaussteuerung (flache Spitzen), auch bei später abgesenktem Pegel | ≥ 1 Stelle = PROBLEM | Vocals | "Aufnahme übersteuert ({n} Stellen)" | Aufnahme: "Gain am Interface ca. −6 dB". Mix: "Declipper auf die Stellen" |
| A02 | Pegel zu niedrig | Peak der aktiven Teile | Peak < −18 dBFS = ACHTUNG | Vocals | "Aufnahme sehr leise (Peaks {p} dBFS)" | "Gain +{−10−p} dB, Ziel: Peaks −10 bis −6 dBFS" |
| A03 | Pegel zu knapp | Anteil Frames mit Peak > −3 dBFS | > 1 % = ACHTUNG | Vocals | "Kaum Reserve nach oben" | "Gain ca. −4 dB" |
| A04 | Rauschen | Grundrauschen in Pausen, A-bewertet, relativ zu Mikro-Erwartung | > −60 dBFS ACHTUNG, > −50 PROBLEM | Vocals | "Rauschen in Pausen ({n} dBFS)" | Aufnahme: Preamp/Mikro-Hinweis (bei dynamischem Mikro: Inline-Vorverstärker). Mix: "Expander/Gate oder Entrauscher" |
| A05 | Brummen | Pausenspektrum: Spitzen bei 50 Hz + Obertönen ≥ 15 dB über Nachbarn | gefunden = ACHTUNG | Vocals | "Brummen bei 50 Hz" | "Notch-Filter 50/100/150 Hz, Kabel/Erdung prüfen" |
| A06 | Raumhall | Blinde Nachhall-Schätzung aus Ausklingphasen | > 0,4 s ACHTUNG, > 0,6 s PROBLEM (nur bei Sicherheit ≥ 0,6) | Vocals | "Raum deutlich hörbar (ca. {t} s)" | Aufnahme: "Raum dämpfen: Absorber/Decken hinter und neben dir; alternativ Mikro mit enger Richtcharakteristik". Mix: "De-Reverb vor dem ersten Kompressor; weniger komprimieren" |
| A07 | Popps | Bass-Stöße < 150 Hz, Anstieg < 10 ms, ≥ 10 dB über lokalem Bass-Mittel | ≥ 2 pro Minute = ACHTUNG | Vocals | "Popps ({n} Stellen)" | Aufnahme: "Popschutz, Mikro leicht seitlich". Mix: "Low Cut 80–100 Hz + Popps per Clip-Gain absenken" |
| A08 | Nahbesprechung | Energie 100–250 Hz gegen Ziel + Mikro-Profil | > +4 dB = ACHTUNG | Vocals | "Viel Bass durch Nähe zum Mikro" | "Low Shelf −{x} dB bei 150 Hz; bei Aufnahme 5–10 cm mehr Abstand" |
| A09 | DC-Offset | Gleichanteil | > 0,5 % FS = ACHTUNG | alle | "Gleichspannungsanteil" | "Hochpass 20 Hz" |
| A10 | Atmer laut | Atmer erkennen (rauschartig zwischen Phrasen), Pegel relativ zur Stimme | > −15 dB = ACHTUNG | LV, DB | "Atmer sehr laut" | "Atmer per Clip-Gain −6 bis −10 dB" |
| A11 | Stereo-Fehler | Korrelation L/R auf Mono-Quelle | < 0,9 ACHTUNG, < −0,3 PROBLEM (Phase gedreht) | Vocals | "Vocal-Spur nicht sauber mono" | "Spur auf Mono stellen (Utility) bzw. Phase eines Kanals drehen" |

## 3. Stufe 2: Sauber

| ID | Check | Messung | Startwert | Rollen | Text | Fix |
|---|---|---|---|---|---|---|
| S01 | Rumpeln | Energie < 80 Hz gegen Ziel | > +3 dB = ACHTUNG | Vocals, BUS | "Rumpeln unter 80 Hz" | "Low Cut bei {f} Hz, 12–18 dB/Okt." |
| S02 | Mumpf | 200–500 Hz gegen Ziel (Fix-Formel) | Überschuss ≥ 1,5 dB | Vocals | "Mumpf {f1}–{f2} Hz" | EQ nach Fix-Formel |
| S03 | Resonanz | Feines (1/24 Okt.) minus geglättetes (1/3 Okt.) Spektrum; Spitze ≥ 6 dB, in ≥ 70 % der aktiven Zeit | gefunden = ACHTUNG, ≥ 10 dB PROBLEM | Vocals | "Stehende Resonanz bei {f} Hz" | "Schmaler Cut −{h−2} dB bei {f} Hz, Q 4–8" |
| S04 | Nasal | 800 Hz–1,5 kHz gegen Ziel | Überschuss ≥ 2 dB | Vocals | "Nasaler Klang um {f} Hz" | EQ nach Fix-Formel |
| S05 | Klicks/Schnitte | Sprünge im Signal an Schnittstellen | ≥ 1 = ACHTUNG | alle | "Klick bei Schnitt ({n})" | "Kurze Fades an den Schnitten" |

## 4. Stufe 3: Kontrolle (Dynamik)

| ID | Check | Messung | Startwert | Rollen | Text | Fix |
|---|---|---|---|---|---|---|
| K01 | Zu viel Dynamik | Streuung Short-term-Lautheit (P90−P10) gegen Ziel | > Ziel-P90 + 2 LU | Vocals | "Lautstärke schwankt zu stark ({x} LU)" | "Kompressor mit ca. {gr} dB Gain Reduction auf lauten Stellen, Ratio 3–4:1" (gr = Überschuss × 0,7) |
| K02 | Überkomprimiert | Crest-Factor 100 ms + 1 s gegen Ziel | < Ziel-P10 − 1,5 dB | Vocals, Drums | "Klingt platt, zu stark komprimiert" | "Gain Reduction um ca. {x} dB verringern oder langsamere Attack" |
| K03 | Wortspitzen | Spitzen > 10 dB über Short-term-Mittel | > 5 pro Minute = ACHTUNG | Vocals | "Einzelne Wörter springen raus" | "Schneller Limiter/Clipper davor oder Clip-Gain" |
| K04 | Pumpen | Pegel des Hintergrunds steigt nach Phrasenende > 6 dB in < 300 ms | ≥ 3 pro Minute = ACHTUNG | Vocals, BUS | "Kompressor pumpt" | "Längere Release oder weniger Gain Reduction" |
| K05 | Transienten weg | Crest-Factor 100 ms gegen Ziel | < Ziel-P10 − 2 dB | KI, SN, Drums | "Anschlag fehlt" | "Attack am Kompressor langsamer (z. B. 20–30 ms)" |
| K06 | Teile ungleich laut | Lautheit Hook vs. Strophe gegen Ziel | Differenz > Ziel + 2 LU | LV | "Hook und Strophe ungleich laut ({x} LU)" | "Clip-Gain/Automation: {Teil} {±x} dB" |

## 5. Stufe 4: Ton & Charakter

| ID | Check | Messung | Startwert | Rollen | Text | Fix |
|---|---|---|---|---|---|---|
| T01 | Zu dunkel | Spektrale Neigung gegen Ziel | < Ziel-P10 − 1 dB/Okt. | Vocals, SUM | "Insgesamt zu dunkel" | "High Shelf +{x} dB ab 5 kHz" |
| T02 | Zu hell | wie T01 | > Ziel-P90 + 1 dB/Okt. | Vocals, SUM | "Insgesamt zu hell" | "High Shelf −{x} dB ab 5 kHz" |
| T03 | Mumpf (gegen Referenz) | wie S02, aber gegen Referenz-Ziel | Überschuss ≥ 1,5 dB | Vocals | "Mumpf {f1}–{f2} Hz" | Fix-Formel |
| T04 | Härte statisch | 2–5 kHz, Überschuss in ≥ 60 % der Zeit | ≥ 1,5 dB | Vocals | "Härte bei {f} Hz" | EQ nach Fix-Formel |
| T05 | Härte dynamisch | 2–5 kHz, Überschuss in < 60 % der Zeit (laute Stellen) | ≥ 2 dB | Vocals | "Härte in lauten Stellen bei {f} Hz" | "Dynamischer EQ −{x} dB bei {f} Hz" |
| T06 | Zischlaute zu laut | S-Ereignisse (Schwerpunkt > 4,5 kHz, < 250 ms), Pegel relativ zu Vokalen gegen Ziel | > Ziel-P90 + 1,5 dB | Vocals | "Zischlaute {x} dB zu laut" | "De-Esser bei ca. {f} kHz, ca. {x} dB Reduktion" |
| T07 | Zischlaute zu stark entfernt | wie T06 | < Ziel-P10 − 3 dB | Vocals | "Klingt gelispelt" | "De-Esser-Reduktion verringern" |
| T08 | Wenig Air | > 10 kHz gegen Ziel | ≥ 2 dB unter Bereich | Vocals, SUM | "Wenig Air/Glanz" | "High Shelf +{x} dB ab 10 kHz (nach dem De-Esser)" |
| T09 | Zu viel Air | > 10 kHz | ≥ 2 dB über Bereich | Vocals, SUM | "Zu viel Glanz, wirkt dünn" | "High Shelf −{x} dB ab 10 kHz" |
| T10 | Wärme fehlt | 120–250 Hz | ≥ 2 dB unter Bereich | Vocals | "Fehlender Körper" | "Low Shelf/Bell +{x} dB bei {f} Hz" |
| T11 | Sättigung hart | nur mit zwei Instanzen: Zunahme Obertöne + Energie 2–5 kHz + Rauheit zwischen den Instanzen | Zunahme 2–5 kHz > 3 dB = ACHTUNG | Vocals | "Die Sättigung macht {f} Hz hart" | "Drive/Mix der Sättigung zurück, oder danach −{x} dB bei {f} Hz" |
| T12 | Rauheit | eine Instanz: schnelle Pegelschwankungen 2–8 kHz (Rauheits-Maß) gegen Ziel | > Ziel-P90 | Vocals | "Klingt rau/kratzig" | "Verzerrung prüfen (Sättigung, Clipper, Aufnahme)" |

## 6. Stufe 5: Im Mix

| ID | Check | Messung | Startwert | Rollen | Text | Fix |
|---|---|---|---|---|---|---|
| M01 | Vocal zu leise | Vocal/Beat-Verhältnis (A-bewertet, 1-s-Fenster, leise Teile verworfen, wie Gerdes & Siedenburg 2023) | < Ziel − 1 dB (Startziel Rap +3,2 dB) | LV | "Vocal sitzt zu leise im Beat ({x} dB, Ziel {z})" | "Vocal +{d} dB" |
| M02 | Vocal zu laut | wie M01 | > Ziel + 2 dB | LV | "Vocal steht zu weit vorne" | "Vocal −{d} dB" |
| M03 | Verdeckung durch Beat | Mithörschwelle des Beats pro Bark-Band (MPEG-Stil), Anteil der Zeit, in der Vocal darunter liegt | Band verdeckt in > 30 % der Zeit = ACHTUNG, > 50 % PROBLEM | LV, AD | "Beat verdeckt Vocal bei {f1}–{f2} Hz" | "Beat: dynamischer EQ −{x} dB bei {f}, ausgelöst vom Vocal (Sidechain)" |
| M04 | 808 vs. Vocal | Überlappung 100–250 Hz | Vocal-Energie dort > Ziel und 808 aktiv | LV, 808 | "808 und Vocal streiten im Bass" | "Low Cut am Vocal auf ca. {f} Hz anheben" |
| M05 | Kick vs. 808 | Gleichzeitige Energie 40–100 Hz bei Kick-Schlägen | Überlappung > Schwelle | KI, 808 | "Kick und 808 überlagern sich" | "808 bei Kick kurz ducken (Sidechain, 3–6 dB)" |
| M06 | Doubles zu laut | Lautheit Double vs. Lead | > Lead − 6 LU | DB | "Doubles fast so laut wie der Lead" | "Doubles −{x} dB" |
| M07 | Doubles Breite | Seite/Mitte der Doubles-Summe | außerhalb Ziel | DB | "Doubles zu breit/zu eng" | "Panorama auf ca. ±{p}" |
| M08 | Adlibs zu laut | Lautheit Adlibs vs. Lead | > Lead − 4 LU | AD | "Adlibs drängen sich vor" | "Adlibs −{x} dB" |
| M09 | Lead nicht mittig | L/R-Ungleichgewicht | > 1 dB | LV | "Lead nicht in der Mitte" | "Panorama auf Mitte" |
| M10 | Mono-Verlust | Pegelverlust bei Mono-Summe pro Band | > 3 dB | BUS, SUM | "Klang fällt in Mono weg bei {f} Hz" | "Stereo-Verbreiterung/Phase bei {f} prüfen" |
| M11 | Bass nicht mono | Seiten-Energie < 120 Hz | > −15 dB relativ Mitte | 808, BT, SUM | "Bass ist nicht mono" | "Unter 120 Hz mono machen (Utility Bass Mono)" |
| M12 | True Peak | True Peak (4× Oversampling) | > −1 dBTP = ACHTUNG, > 0 PROBLEM | SUM | "Spitzen über {x} dBTP" | "Limiter Ceiling −1 dBTP" |
| M13 | Lautheit Summe | Integrated gegen Referenz | Differenz > 2 LU (nur Info) | SUM | "Mix {x} LU leiser/lauter als Referenz" | "Info, kein Fehler: fürs Mastering" |
| M14 | Klangbalance Summe | 4 Bänder (<250, 250–2k, 2–8k, >8k) gegen Referenz | ±3 dB außerhalb Bereich | SUM | "Mix im {Band} {x} dB über/unter Referenz" | "Breiter EQ {±x} dB im {Band}" |

## 7. System-Meldungen

| ID | Meldung | Wann |
|---|---|---|
| G01 | "Zu wenig Daten, weiter abspielen" | Bedingung 4 nicht erfüllt |
| G02 | "Nicht verbunden" | keine anderen Instanzen bzw. Hilfsprogramm nicht erreichbar; Beziehungs-Checks (M-Gruppe) pausieren |
| G03 | "Referenz-Ordner hat zu wenig Songs ({n}/10)" | < 10 Songs im gewählten Ordner; Literatur-Startwerte werden verwendet |
| G04 | "Analyse pausiert (Export/Transport steht)" | Offline-Render oder Stopp |
| G05 | "Spur scheint stumm" | Master-Abgleich findet Spur nicht in der Summe |

## 8. Was Phase 1 messen können muss (aus dem Katalog abgeleitet)

Lautheit BS.1770 (M/S/I, LRA) · True Peak · 1/6- und 1/24-Oktav-Spektrum mit Zeitstatistik · spektrale Neigung · Crest-Factor 100 ms/1 s · Aktivitäts-/Sprach-Erkennung · S-Ereignis-Erkennung · Atmer-Erkennung · Popp-Erkennung · Clipping-Erkennung (flache Spitzen) · Brumm-Erkennung · blinde Nachhall-Schätzung · Rauheits-Maß · Korrelation/Mitte-Seite pro Band · A-bewertetes Vocal/Beat-Verhältnis · Bark-Mithörschwelle (Maskierung) · Teil-Erkennung Hook/Strophe.
