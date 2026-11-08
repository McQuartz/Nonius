# Phase 0: Ergebnisse erster Test (2026-11-08)

Host: Ableton Live 12, Windows, 48 kHz, Blockgröße 128. Sechs Log-Dateien, fünf Instanzen gleichzeitig (Spuren „neu“, „newuu“, „neu“, „Main“ = Master, später „beat2“).

| Frage | Ergebnis |
|---|---|
| Lädt das Plugin? | Ja, in Live 12 (nicht in Live 10.0.1, das Bundle-Format zu alt). Latenz 0. |
| Läuft processBlock bei Stopp? | Ja, durchgehend ~375 Blöcke/s (= 48000/128), auch gestoppt. |
| Gemeinsame Zeitbasis aller Instanzen? | Ja. Alle Instanzen melden im selben Block dieselbe `time_in_samples`, Wanduhr-Abweichung < 4 ms. Grundlage für den Vocal-gegen-Beat-Vergleich steht. |
| Play/Stopp erkennen | Ja, alle Instanzen gleichzeitig (Abweichung ~2 ms). |
| Export erkennen | Ja, `OFFLINE_RENDER_START/ENDE` in allen Instanzen, mit RELEASE/PREPARE davor und danach. |
| Spurname/-farbe | Ja, kommt an, auch Umbenennen und Farbwechsel live. Name und Farbe kommen in getrennten Meldungen. |
| Shared Memory zwischen Instanzen | Ja, erste Instanz legt an, alle weiteren öffnen. |
| Gerät aus (Test 2) | Live ruft processBlock gar nicht mehr auf (0 Blöcke/s), die Instanz auf dem Master läuft weiter. Nonius erkennt „Gerät aus“ daran, dass keine Blöcke mehr kommen. |
| Spur stumm | Noch nicht eindeutig gemessen, im ersten Test liefen alle Instanzen weiter. Nicht kritisch: Die Master-Instanz „Summe“ erkennt stumme Spuren ohnehin. |
| Fenster verschwindet bei Spurwechsel | Ja, laut Ace springen die Fenster mit der gewählten Spur um (Auto-Hide). Die Probe hat das nicht erkannt (`isShowing()`), Fix mit HWND-Prüfung ist gebaut, aber noch nicht getestet (Test 2 lief mit altem Build). |
| Grundrauschen Master | Bei Stopp ca. −63 dBFS auf dem Master (vermutlich Analog-Emulation). Nonius braucht eine Stille-Schwelle, die das berücksichtigt. |
| Master-Peak | bis +4,8 dBFS (Float, kein Clipping im Plugin), Nonius muss > 0 dBFS anzeigen können. |

## Fehler in der Probe

- Instanz-Nummer war immer 1: Der `SharedResourcePointer` war nur eine lokale Variable und wurde sofort wieder gelöscht. Behoben durch eine statische Registry pro DLL.
- Fenster-Sichtbarkeit: siehe oben, zweiter Test nötig.

## Folgerungen für Phase 1+

- Gemeinsame Zeitbasis über `time_in_samples` funktioniert, Instanzen können Messfenster sample-genau abgleichen.
- Bei Stopp läuft Audio weiter (Stille): Messungen bei `spielt=0` verwerfen bzw. einfrieren.
- Export wird erkannt: während Offline-Render keine Live-Anzeige, optional Messung „vom Export“.
