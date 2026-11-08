# Phase 0: Test mit „Nonius Probe“ in Ableton

Dauer: ca. 15 Minuten. Das Test-Plugin verändert den Klang nicht. Es schreibt nur mit, wie Ableton sich verhält. Ergebnis: Log-Dateien, die du mir schickst.

## 1. Installieren
1. Auf GitHub im Repo oben auf **Actions** → letzter grüner Lauf „Build (Windows)“ → unten unter *Artifacts* **NoniusProbe-VST3-Windows** herunterladen und entpacken.
2. Den Ordner **„Nonius Probe.vst3“** nach `C:\Program Files\Common Files\VST3\` kopieren.
3. Ableton starten. Falls es nicht auftaucht: Einstellungen → Plug-Ins → „VST3-Plug-In-Systemordner verwenden“ an → „Erneut scannen“.

Die Log-Dateien landen in `Dokumente\NoniusProbe\` (eine Datei pro Instanz).

## 2. Vorbereitung
- Neues Projekt mit 3 Audiospuren und etwas Audio darauf (z. B. Vocal, Beat, irgendein Loop). Spuren benennen: **Vocal**, **Beat**, **Test**.
- Auf jede Spur und auf den **Master** je einmal „Nonius Probe“ ziehen (also 4 Instanzen).
- Auf **Vocal** zusätzlich irgendein Plugin mit Latenz (z. B. einen Limiter mit Lookahead oder Abletons Glue Compressor) und **dahinter** eine zweite „Nonius Probe“.

Im Plugin-Fenster gibt es den Knopf **„Marker setzen“**. Drück ihn **vor jedem Schritt** unten einmal, auf irgendeiner Instanz. So kann ich im Log sehen, was wann passiert ist.

## 3. Ablauf
| Schritt | Was tun | Was wir lernen |
|---|---|---|
| 1 | Marker. 20 s abspielen. | Grundwerte, Zeitposition aller Instanzen (Frage 4) |
| 2 | Marker. Stopp, 10 s warten. | Läuft das Plugin bei Stopp weiter? (Frage 3) |
| 3 | Marker. Spur **Test** stummschalten (Spur-Aktivator aus), 10 s abspielen. | Läuft es auf stummen Spuren weiter? (Frage 2) |
| 4 | Marker. Spur **Test** wieder an. Auf Test die Probe per Geräte-Schalter deaktivieren, 10 s abspielen, wieder aktivieren. | Verhalten bei deaktiviertem Gerät |
| 5 | Marker. Probe-Fenster auf dem **Master** öffnen. Dann Spur **Vocal** anklicken, dann wieder Master. | Wann verschwindet das Fenster? (Frage 1) |
| 6 | Marker. Spur **Beat** umbenennen in **Beat2** und Farbe ändern. | Kommt der Spurname an? (Frage 5) |
| 7 | Marker. 8 Takte exportieren (Datei → Audio/Video exportieren). | Verhalten beim Export |
| 8 | Ableton schließen. | Sauberes Beenden |

## 4. Zurückschicken
Den ganzen Ordner `Dokumente\NoniusProbe\` als ZIP in den Projekt-Ordner `B:\Claude Projects\Mixing VST\` legen und mir Bescheid geben. Ich werte es aus.

Was zusätzlich hilft: kurz notieren, ob dir bei Schritt 5 etwas aufgefallen ist (z. B. „Fenster blieb offen“).
