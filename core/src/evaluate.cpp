// Hinweis-Katalog v0.1 auf die Messwerte anwenden.
// Grenzwerte sind Startwerte aus docs/hinweis-katalog.md. Checks, die ein Ziel aus Referenzen
// brauchen (S02, S04, T-Gruppe), werden als "Info" mit Messwert ausgegeben, bis Referenzen da sind.
#include "nonius/analysis.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace nonius
{
    namespace
    {
        std::string fmt (const char* f, double v)
        {
            char buf[64];
            std::snprintf (buf, sizeof (buf), f, v);
            return buf;
        }

        std::string fmt2 (const char* f, double a, double b)
        {
            char buf[96];
            std::snprintf (buf, sizeof (buf), f, a, b);
            return buf;
        }

        double roundHalf (double v) { return std::round (v * 2.0) / 2.0; }

        double niceFrequency (double f)
        {
            const double step = f < 200 ? 10 : f < 1000 ? 20 : f < 5000 ? 100 : 500;
            return std::round (f / step) * step;
        }

        std::string hz (double f)
        {
            f = niceFrequency (f);
            return f >= 1000 ? fmt ("%.1f kHz", f / 1000.0) : fmt ("%.0f Hz", f);
        }

        // Gehörgewicht aus dem Katalog
        double earWeight (double f)
        {
            if (f >= 1000 && f <= 5000) return 1.0;
            if ((f >= 500 && f < 1000) || (f > 5000 && f <= 10000)) return 0.8;
            if ((f >= 100 && f < 500) || f > 10000) return 0.6;
            return 0.4;
        }

        Finding make (const char* id, const char* title, Severity s, std::string text, std::string fix,
                      std::string measured, double priority = 0.0, std::string note = {})
        {
            Finding f;
            f.id = id; f.title = title; f.severity = s;
            f.text = std::move (text); f.fix = std::move (fix); f.measured = std::move (measured);
            f.priority = priority; f.note = std::move (note);
            return f;
        }
    }

    const char* severityLabel (Severity s)
    {
        switch (s)
        {
            case Severity::ok:      return "OK";
            case Severity::warning: return "ACHTUNG";
            case Severity::problem: return "PROBLEM";
            case Severity::noData:  return "KEINE DATEN";
            case Severity::info:    return "INFO";
        }
        return "?";
    }

    const char* roleLabel (Role r)
    {
        switch (r)
        {
            case Role::leadVocal:  return "Lead Vocal";
            case Role::vocalGroup: return "Vocal-Gruppe";
            case Role::beat:       return "Beat";
            case Role::sum:        return "Summe";
        }
        return "?";
    }

    // ==================================================================
    std::vector<Finding> evaluateVocal (const TrackAnalysis& t, bool raw)
    {
        std::vector<Finding> out;
        const std::string mp3Note = t.lossySource ? "MP3: eingeschränkt messbar" : "";

        if (t.activeSeconds < 8.0)
        {
            out.push_back (make ("G01", "Zu wenig Daten", Severity::noData, "Zu wenig Gesang (unter 8 s)", "", fmt ("%.1f s", t.activeSeconds)));
            return out;
        }

        // ---------------- Stufe 1: Aufnahme (nur sinnvoll auf der Rohspur)
        if (raw)
        {
            // A01 Clipping
            if (t.clippingRuns > 0)
                out.push_back (make ("A01", "Clipping", Severity::problem,
                                     "Aufnahme übersteuert (" + std::to_string (t.clippingRuns) + " Stellen)",
                                     "Aufnahme: Gain am Interface ca. −6 dB. Mix: Declipper auf die Stellen",
                                     std::to_string (t.clippingRuns) + " Stellen", 10.0, mp3Note));
            else
                out.push_back (make ("A01", "Clipping", Severity::ok, "Keine flachen Spitzen gefunden", "", "0 Stellen", 0, mp3Note));

            // A02 / A03 Pegel
            if (t.activePeakDb < -18.0)
                out.push_back (make ("A02", "Pegel zu niedrig", Severity::warning,
                                     fmt ("Aufnahme sehr leise (Peaks %.1f dBFS)", t.activePeakDb),
                                     fmt ("Gain +%.0f dB, Ziel: Peaks −10 bis −6 dBFS", -10.0 - t.activePeakDb),
                                     fmt ("%.1f dBFS", t.activePeakDb), (-18.0 - t.activePeakDb)));
            else if (t.hotFramePercent > 1.0)
                out.push_back (make ("A03", "Pegel zu knapp", Severity::warning, "Kaum Reserve nach oben",
                                     "Gain ca. −4 dB", fmt ("%.1f %% der Zeit über −3 dBFS", t.hotFramePercent), 3.0));
            else
                out.push_back (make ("A02", "Aufnahmepegel", Severity::ok, fmt ("Peaks bei %.1f dBFS", t.activePeakDb), "", fmt ("%.1f dBFS", t.activePeakDb)));

            // A04 Rauschen
            if (! t.noiseFloorReliable)
                out.push_back (make ("A04", "Rauschen", Severity::noData, "Zu wenig Pausen zum Messen", "", "–"));
            else if (t.noiseFloorDbA <= -149.0)
                out.push_back (make ("A04", "Rauschen", Severity::info, "Pausen sind digital still (geschnitten oder Gate)", "", "digitale Stille"));
            else
            {
                const auto s = t.noiseFloorDbA > -50.0 ? Severity::problem : t.noiseFloorDbA > -60.0 ? Severity::warning : Severity::ok;
                out.push_back (make ("A04", "Rauschen", s, fmt ("Rauschen in Pausen (%.0f dBFS A)", t.noiseFloorDbA),
                                     s == Severity::ok ? "" : "Aufnahme: Preamp-Gain und Mikro prüfen (beim SM7B: Inline-Vorverstärker). Mix: Expander/Gate oder Entrauscher",
                                     fmt ("%.1f dBFS(A)", t.noiseFloorDbA), std::max (0.0, t.noiseFloorDbA + 60.0)));
            }

            // A05 Brummen
            if (t.humExcessDb >= 15.0)
                out.push_back (make ("A05", "Brummen", Severity::warning, fmt ("Brummen bei %.0f Hz", t.humFrequency),
                                     fmt2 ("Notch-Filter %.0f/%.0f Hz (+ 3. Oberton), Kabel/Erdung prüfen", t.humFrequency, 2 * t.humFrequency),
                                     fmt ("%.0f dB über Nachbarn", t.humExcessDb), 4.0));

            // A06 Raumhall (experimentell)
            if (t.reverbDecays < 5)
                out.push_back (make ("A06", "Raumhall", Severity::noData, "Zu wenige freie Ausklingvorgänge zum Schätzen", "",
                                     std::to_string (t.reverbDecays) + " Ausklänge", 0, "experimentell"));
            else
            {
                Severity s = t.reverbSeconds > 0.6 ? Severity::problem : t.reverbSeconds > 0.4 ? Severity::warning : Severity::ok;
                if (t.reverbConfidence < 0.6 && s != Severity::ok) s = Severity::info;
                out.push_back (make ("A06", "Raumhall", s, fmt ("Raum hörbar (Nachhall ca. %.2f s)", t.reverbSeconds),
                                     s == Severity::ok ? "" : "Aufnahme: Raum dämpfen (Absorber hinter und neben dir, Decke über dir), näher ans Mikro. Mix: De-Reverb vor dem ersten Kompressor, weniger komprimieren",
                                     fmt2 ("%.2f s (Sicherheit %.1f)", t.reverbSeconds, t.reverbConfidence),
                                     std::max (0.0, (t.reverbSeconds - 0.4) * 20.0), "experimentell, noch nicht kalibriert"));
            }

            // A07 Popps
            {
                const auto s = t.plosives.perMinute >= 2.0 ? Severity::warning : Severity::ok;
                out.push_back (make ("A07", "Popps", s, "Popps (" + std::to_string (t.plosives.count) + " Stellen)",
                                     s == Severity::ok ? "" : "Aufnahme: Popschutz, Mikro leicht seitlich. Mix: Low Cut 80–100 Hz und Popps per Clip-Gain absenken",
                                     fmt ("%.1f pro Minute", t.plosives.perMinute), t.plosives.perMinute));
            }

            // A11 Stereo-Fehler (nur Rohspur, bearbeitete Spuren dürfen Stereo-Effekte haben)
            if (t.channels >= 2 && t.correlation < 0.9)
            {
                const auto s = t.correlation < -0.3 ? Severity::problem : Severity::warning;
                out.push_back (make ("A11", "Stereo-Fehler", s, "Vocal-Spur ist nicht sauber mono",
                                     s == Severity::problem ? "Phase eines Kanals drehen (Utility)" : "Spur auf Mono stellen (Utility)",
                                     fmt ("Korrelation %.2f", t.correlation), 5.0));
            }
        }

        // A09 DC
        if (t.dcOffsetPercent > 0.5)
            out.push_back (make ("A09", "DC-Offset", Severity::warning, "Gleichspannungsanteil", "Hochpass 20 Hz",
                                 fmt ("%.2f %% FS", t.dcOffsetPercent), 2.0));

        // ---------------- Stufe 2: Sauber
        {
            // S01 Rumpeln: Startziel −28 dB relativ (Energie < 80 Hz gegen 200 Hz–5 kHz), Hinweis ab +3 dB
            const double excess = t.rumbleDb - (-28.0);
            const auto s = excess > 3.0 ? Severity::warning : Severity::ok;
            out.push_back (make ("S01", "Rumpeln", s, fmt ("Energie unter 80 Hz: %.0f dB relativ zur Stimme", t.rumbleDb),
                                 s == Severity::ok ? "" : "Low Cut bei 80–100 Hz, 12–18 dB/Okt.",
                                 fmt ("%.1f dB", t.rumbleDb), std::max (0.0, excess) * 0.4, "Startziel −28 dB, wird kalibriert"));
        }

        // S03 Resonanzen
        if (t.resonances.empty())
            out.push_back (make ("S03", "Resonanz", Severity::ok, "Keine stehenden Resonanzen gefunden", "", "–"));
        for (const auto& r : t.resonances)
        {
            const auto s = r.heightDb >= 10.0 ? Severity::problem : Severity::warning;
            out.push_back (make ("S03", "Resonanz", s, "Stehende Resonanz bei " + hz (r.frequency),
                                 fmt ("Schmaler Cut −%.1f dB bei ", roundHalf (r.heightDb - 2.0)) + hz (r.frequency) + ", Q 4–8",
                                 fmt2 ("+%.1f dB, %.0f %% der Zeit", r.heightDb, r.persistence * 100.0),
                                 r.heightDb * earWeight (r.frequency)));
        }

        // S02/S04 brauchen Zielkurven
        out.push_back (make ("S02", "Mumpf (200–500 Hz)", Severity::noData, "Braucht Zielkurve aus Referenzen", "",
                             fmt ("100–250 Hz: %.1f dB relativ", t.proximityDb)));

        // ---------------- Stufe 3: Kontrolle
        const double spread = t.loudness.shortTermP90 - t.loudness.shortTermP10;
        if (! raw)
        {
            // K01: Startziel für fertige Rap-Vocals: Streuung ≤ 6 LU, Hinweis ab +2 LU
            const auto s = spread > 8.0 ? Severity::warning : Severity::ok;
            out.push_back (make ("K01", "Dynamik", s, fmt ("Lautstärke schwankt um %.1f LU", spread),
                                 s == Severity::ok ? "" : fmt ("Kompressor mit ca. %.0f dB Gain Reduction auf lauten Stellen, Ratio 3–4:1", roundHalf ((spread - 6.0) * 0.7)),
                                 fmt ("%.1f LU (P90−P10)", spread), std::max (0.0, spread - 6.0), "Startziel ≤ 6 LU, wird mit Referenzen ersetzt"));

            // K02: überkomprimiert, Startwert Crest 100 ms < 6,5 dB
            const auto s2 = t.crest100msDb < 6.5 ? Severity::warning : Severity::ok;
            out.push_back (make ("K02", "Überkomprimiert", s2, fmt ("Crest-Faktor %.1f dB (100 ms)", t.crest100msDb),
                                 s2 == Severity::ok ? "" : "Gain Reduction verringern oder langsamere Attack",
                                 fmt2 ("%.1f dB (100 ms), %.1f dB (1 s)", t.crest100msDb, t.crest1sDb), std::max (0.0, 6.5 - t.crest100msDb),
                                 "Startwert, wird mit Referenzen ersetzt"));
        }
        else
        {
            out.push_back (make ("K01", "Dynamik (roh)", Severity::info, fmt ("Rohspur schwankt um %.1f LU", spread), "",
                                 fmt ("%.1f LU (P90−P10)", spread)));
        }

        {
            const auto s = t.wordPeaks.perMinute > 5.0 ? Severity::warning : Severity::ok;
            out.push_back (make ("K03", "Wortspitzen", s, std::to_string (t.wordPeaks.count) + " einzelne Wörter springen raus",
                                 s == Severity::ok ? "" : "Schneller Limiter/Clipper vor dem Kompressor oder Clip-Gain auf die Stellen",
                                 fmt ("%.1f pro Minute", t.wordPeaks.perMinute), t.wordPeaks.perMinute * 0.5));
        }

        // ---------------- Stufe 4: Ton
        if (t.sibilantToVowelDb > -149.0)
        {
            // Startwerte: S-Laute über −4 dB relativ zu Vokalen = zu scharf, unter −22 dB = gelispelt
            Severity s = Severity::ok;
            std::string text = fmt ("Zischlaute %.1f dB relativ zu Vokalen", t.sibilantToVowelDb);
            std::string fix;
            if (t.sibilantToVowelDb > -4.0)
            {
                s = t.sibilantToVowelDb > 0.0 ? Severity::problem : Severity::warning;
                fix = "De-Esser bei ca. " + hz (t.sibilantFrequency) + fmt (", ca. %.0f dB Reduktion", roundHalf (t.sibilantToVowelDb + 8.0));
            }
            else if (! raw && t.sibilantToVowelDb < -22.0)
            {
                s = Severity::warning;
                text = "Klingt evtl. gelispelt (" + text + ")";
                fix = "De-Esser-Reduktion verringern";
            }
            out.push_back (make (t.sibilantToVowelDb < -22.0 ? "T07" : "T06", "Zischlaute", s, text, fix,
                                 fmt ("%.1f dB, ", t.sibilantToVowelDb) + std::to_string (t.sibilants.count) + " S-Laute, Schwerpunkt " + hz (t.sibilantFrequency),
                                 std::max (0.0, t.sibilantToVowelDb + 4.0), "Startwerte, werden kalibriert"));
        }
        else
            out.push_back (make ("T06", "Zischlaute", Severity::noData, "Zu wenige S-Laute oder Vokale erkannt", "", "–"));

        out.push_back (make ("T01", "Helligkeit", Severity::info, fmt ("Spektrale Neigung %.1f dB/Oktave", t.spectrum.tiltDbPerOct),
                             "", fmt ("%.1f dB/Okt.", t.spectrum.tiltDbPerOct), 0, "Bewertung braucht Referenzen"));
        if (t.airDb > -149.0)
            out.push_back (make ("T08", "Air", Severity::info, fmt ("Energie über 10 kHz: %.1f dB relativ", t.airDb), "",
                                 fmt ("%.1f dB", t.airDb), 0, t.lossySource ? "MP3 schneidet oft über 16 kHz ab" : "Bewertung braucht Referenzen"));

        // M09 Lead mittig (bearbeitete Spur)
        if (! raw && t.channels >= 2 && std::fabs (t.balanceDb) > 1.0)
            out.push_back (make ("M09", "Lead nicht mittig", Severity::warning, fmt ("Lead %.1f dB zur Seite", t.balanceDb),
                                 "Panorama auf Mitte prüfen", fmt ("L−R %.1f dB", t.balanceDb), std::fabs (t.balanceDb)));

        return out;
    }

    // ==================================================================
    std::vector<Finding> evaluateBeat (const TrackAnalysis& t)
    {
        std::vector<Finding> out;
        {
            const auto s = t.truePeakDb > 0.0 ? Severity::problem : t.truePeakDb > -1.0 ? Severity::warning : Severity::ok;
            out.push_back (make ("M12", "True Peak", s, fmt ("Spitzen bei %.1f dBTP", t.truePeakDb),
                                 s == Severity::ok ? "" : "Beat-Spur ca. 3 dB leiser ziehen, damit Platz für das Vocal bleibt",
                                 fmt ("%.1f dBTP", t.truePeakDb), std::max (0.0, t.truePeakDb + 1.0)));
        }
        if (t.channels >= 2)
        {
            const auto s = t.sideBelow120Db > -15.0 ? Severity::warning : Severity::ok;
            out.push_back (make ("M11", "Bass mono", s, fmt ("Seite unter 120 Hz: %.0f dB relativ zur Mitte", t.sideBelow120Db),
                                 s == Severity::ok ? "" : "Unter 120 Hz mono machen (Utility: Bass Mono)",
                                 fmt ("%.1f dB", t.sideBelow120Db), std::max (0.0, t.sideBelow120Db + 15.0)));

            const auto s2 = t.monoLossMaxDb > 3.0 ? Severity::warning : Severity::ok;
            out.push_back (make ("M10", "Mono-Verlust", s2, "Größter Verlust in Mono bei " + hz (t.monoLossFrequency),
                                 s2 == Severity::ok ? "" : "Stereo-Verbreiterung/Phase in diesem Bereich prüfen",
                                 fmt ("%.1f dB", t.monoLossMaxDb), std::max (0.0, t.monoLossMaxDb - 3.0)));
        }
        out.push_back (make ("INFO", "Lautheit Beat", Severity::info, fmt ("%.1f LUFS integriert", t.loudness.integratedLufs), "",
                             fmt2 ("%.1f LUFS, LRA %.1f LU", t.loudness.integratedLufs, t.loudness.lra)));
        return out;
    }

    // ==================================================================
    std::vector<Finding> evaluateMix (const MixRelation& m, const TrackAnalysis& vocal, const TrackAnalysis& beat)
    {
        std::vector<Finding> out;
        constexpr double target = 3.2; // Gerdes & Siedenburg 2023, Rap

        if (m.windowsUsed < 8)
            out.push_back (make ("G01", "Zu wenig Daten", Severity::noData, "Zu wenig gemeinsame Zeit von Vocal und Beat", "",
                                 std::to_string (m.windowsUsed) + " s"));
        else
        {
            const double d = m.vocalToBeatDb - target;
            Severity s = Severity::ok; const char* id = "M01"; std::string fix;
            if (d < -1.0) { s = d < -4.0 ? Severity::problem : Severity::warning; fix = fmt ("Vocal +%.1f dB", roundHalf (-d)); }
            else if (d > 2.0) { s = d > 5.0 ? Severity::problem : Severity::warning; id = "M02"; fix = fmt ("Vocal −%.1f dB", roundHalf (d)); }
            out.push_back (make (id, "Vocal gegen Beat", s,
                                 fmt2 ("Vocal liegt %.1f dB über dem Beat (Ziel %.1f dB)", m.vocalToBeatDb, target), fix,
                                 fmt2 ("%.1f dB (P10–P90 %.1f", m.vocalToBeatDb, m.vocalToBeatP10) + fmt (" bis %.1f dB)", m.vocalToBeatP90),
                                 std::fabs (d) * 1.5, "Startziel aus der Literatur, wird durch deine Referenzen ersetzt"));
        }

        if (m.maskedRegions.empty())
            out.push_back (make ("M03", "Verdeckung", Severity::ok, "Der Beat verdeckt das Vocal in keinem Bereich dauerhaft", "", "–"));
        for (const auto& r : m.maskedRegions)
        {
            const auto s = r.maskedFraction > 0.5 ? Severity::problem : Severity::warning;
            const double fc = std::sqrt (r.lowHz * r.highHz);
            out.push_back (make ("M03", "Verdeckung", s, "Beat verdeckt Vocal bei " + hz (r.lowHz) + "–" + hz (r.highHz),
                                 fmt ("Beat: dynamischer EQ −%.1f dB bei ", std::clamp (roundHalf (r.neededDb), 1.0, 6.0)) + hz (fc) + ", per Sidechain vom Vocal ausgelöst",
                                 fmt ("%.0f %% der Zeit verdeckt", r.maskedFraction * 100.0) + fmt (", typisch %.1f dB unter der Schwelle", r.neededDb),
                                 r.maskedFraction * 10.0 * earWeight (fc), "vereinfachtes Hörmodell, wird kalibriert"));
        }

        if (m.sumTruePeakDb > -149.0)
            out.push_back (make ("INFO", "Summe Vocal + Beat", Severity::info,
                                 fmt2 ("Summe ohne Master: %.1f LUFS, True Peak %.1f dBTP", m.sumIntegratedLufs, m.sumTruePeakDb), "", "–"));

        (void) vocal; (void) beat;
        return out;
    }
}
