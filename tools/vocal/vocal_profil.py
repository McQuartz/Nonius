#!/usr/bin/env python3
"""NONIUS Vocal-Profil: Referenz-Songs mit HTDemucs in Vocal + Beat trennen, die Stems mit
nonius-vocal messen und daraus Zielbereiche (P10/P50/P90) für die Rollen "Vocal bearbeitet"
und "Vocal roh" bauen.

Schritte (einzeln aufrufbar):
  trennen  --ordner REFERENZEN --stems STEMS        (htdemucs, --two-stems=vocals)
  messen   --stems STEMS --tool nonius-vocal --werte werte.jsonl
  profil   --werte werte.jsonl --favoriten "a;b" --nur-vergleich "c" --profil vocal-profil.json
           [--korrektur korrektur.json]

Favoriten zählen doppelt (wie tools/reference), "nur Vergleich" wird gemessen, aber nicht ins
Profil gerechnet. --korrektur: systematische Fehler der Stem-Trennung (aus der MUSDB18-Prüfung),
werden vor den Perzentilen abgezogen.
"""
import argparse
import json
import math
import os
import subprocess
import sys

AUDIO = (".mp3", ".wav", ".flac", ".m4a", ".aif", ".aiff", ".ogg")
CENTRES = [31.5, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]

# Messwerte im Profil: Schlüssel in werte.jsonl -> (Label im Profil, Einheit)
FIELDS = [
    ("silbe90_30", "Silben-Dynamik", "dB"),
    ("silbe95_50", "Silben-Spitzen", "dB"),
    ("streuung", "Lautstärke-Schwankung", "LU"),
    ("crest100", "Crest 100 ms", "dB"),
    ("crest1s", "Crest 1 s", "dB"),
    ("zischS", "Zischlaute (S zu Vokal)", "dB"),
    ("luecke", "Abstand Wort zu Lücke", "dB"),
    ("raumNachklang", "Nachklang nach Wortende", "dB"),
    ("neigung", "Spektrale Neigung", "dB/Okt."),
    ("tief", "Bass < 250 Hz", "dB"),
    ("tiefeMitten", "Tiefe Mitten 250 Hz–2 kHz", "dB"),
    ("hoheMitten", "Hohe Mitten 2–8 kHz", "dB"),
    ("hoehen", "Höhen > 8 kHz", "dB"),
    ("breite", "Breite (Seite zu Mitte)", "dB"),
    ("vocalZuBeat", "Vocal über Beat", "dB"),
]


def matches_any(name, spec):
    return any(p.strip() and p.strip().lower() in name.lower() for p in (spec or "").split(";"))


def percentile(values, p):
    v = sorted(values)
    if not v:
        return float("nan")
    k = (len(v) - 1) * p / 100.0
    lo, hi = math.floor(k), math.ceil(k)
    return v[lo] + (v[hi] - v[lo]) * (k - lo)


def weighted(rows, key, idx=None):
    out = []
    for r in rows:
        v = r.get(key)
        if idx is not None:
            v = v[idx] if v else None
        if v is None or not isinstance(v, (int, float)) or v < -140 or math.isnan(v):
            continue
        out += [v] * int(round(r["gewicht"]))
    return out


def cmd_trennen(a):
    os.makedirs(a.stems, exist_ok=True)
    files = sorted(f for f in os.listdir(a.ordner) if f.lower().endswith(AUDIO))
    for f in files:
        name = os.path.splitext(f)[0]
        if os.path.exists(os.path.join(a.stems, "htdemucs", name, "vocals.wav")):
            continue
        print("trenne", f, flush=True)
        args = [sys.executable, "-m", "demucs", "-n", "htdemucs", "--two-stems=vocals",
                "-o", a.stems, "-j", str(a.jobs)]
        if a.segment:
            args += ["--segment", str(a.segment)]
        subprocess.run(args + [os.path.join(a.ordner, f)], check=True)


def cmd_messen(a):
    base = os.path.join(a.stems, "htdemucs")
    with open(a.werte, "w", encoding="utf-8") as out:
        for name in sorted(os.listdir(base)):
            d = os.path.join(base, name)
            v, b = os.path.join(d, "vocals.wav"), os.path.join(d, "no_vocals.wav")
            if not os.path.exists(v):
                continue
            r = subprocess.run([a.tool, "--vocal", v, "--beat", b, "--name", name],
                               capture_output=True, text=True, check=True)
            out.write(json.dumps(json.loads(r.stdout), ensure_ascii=False) + "\n")
            print("gemessen", name, flush=True)


def load_rows(path, favs, only):
    rows = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.strip():
                r = json.loads(line)
                r["gewicht"] = 0 if matches_any(r["name"], only) else (2 if matches_any(r["name"], favs) else 1)
                rows.append(r)
    return rows


def apply_correction(rows, corr):
    for r in rows:
        for k, off in corr.get("felder", {}).items():
            if isinstance(r.get(k), (int, float)) and r[k] > -140:
                r[k] -= off
        if "oktaven" in corr and r.get("oktaven"):
            r["oktaven"] = [x - o if x > -140 else x for x, o in zip(r["oktaven"], corr["oktaven"])]


def cmd_profil(a):
    rows = load_rows(a.werte, a.favoriten, a.nur_vergleich)
    if a.korrektur:
        with open(a.korrektur, encoding="utf-8") as f:
            apply_correction(rows, json.load(f))
    prof = [r for r in rows if r["gewicht"] > 0]
    ranges = {}
    for key, label, unit in FIELDS:
        vals = weighted(prof, key)
        if len(vals) >= 5:
            ranges[label] = [round(percentile(vals, p), 2) for p in (10, 50, 90)]
    octs = []
    for i, c in enumerate(CENTRES):
        vals = weighted(prof, "oktaven", i)
        octs.append([c] + [round(percentile(vals, p), 2) for p in (10, 50, 90)])
    result = {
        "version": 1,
        "rolle": "Vocal bearbeitet",
        "quelle": "HTDemucs-Stems der Referenz-Songs",
        "songs": len(prof),
        "bereiche": ranges,
        "oktaven": octs,
        "einheiten": {label: unit for _, label, unit in FIELDS},
    }
    with open(a.profil, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=1)
    print(json.dumps(result, ensure_ascii=False, indent=1))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    t = sub.add_parser("trennen")
    t.add_argument("--ordner", required=True)
    t.add_argument("--stems", required=True)
    t.add_argument("--jobs", type=int, default=1)
    t.add_argument("--segment", type=float, default=0)
    m = sub.add_parser("messen")
    m.add_argument("--stems", required=True)
    m.add_argument("--tool", required=True)
    m.add_argument("--werte", required=True)
    p = sub.add_parser("profil")
    p.add_argument("--werte", required=True)
    p.add_argument("--favoriten", default="")
    p.add_argument("--nur-vergleich", dest="nur_vergleich", default="")
    p.add_argument("--profil", required=True)
    p.add_argument("--korrektur", default="")
    a = ap.parse_args()
    {"trennen": cmd_trennen, "messen": cmd_messen, "profil": cmd_profil}[a.cmd](a)


if __name__ == "__main__":
    main()
