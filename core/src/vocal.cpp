// Vocal gegen das Vocal-Profil aus den Referenzen (HTDemucs-Stems).
// Kalibrierung der Kompressions-Umrechnung: docs/vocal-profil.md (Kompressor-Sweep auf Rohspuren).
#include "nonius/profile.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace nonius
{
    namespace
    {
        std::string fmt (const char* f, double v)
        {
            char buf[128];
            std::snprintf (buf, sizeof (buf), f, v);
            return buf;
        }

        std::string fmt2 (const char* f, double a, double b)
        {
            char buf[160];
            std::snprintf (buf, sizeof (buf), f, a, b);
            return buf;
        }

        double roundHalf (double v) { return std::round (v * 2.0) / 2.0; }

        std::string hzLabel (double f)
        {
            if (f >= 1000) return std::fmod (f, 1000.0) == 0 ? fmt ("%.0f kHz", f / 1000.0) : fmt ("%.1f kHz", f / 1000.0);
            return fmt ("%.0f Hz", f);
        }

        // Komma statt Punkt in Zahlen (deutsche Anzeige), nur in den fertigen Texten
        std::string de (std::string s)
        {
            for (std::size_t i = 1; i + 1 < s.size(); ++i)
                if (s[i] == '.' && std::isdigit ((unsigned char) s[i - 1]) && std::isdigit ((unsigned char) s[i + 1])) s[i] = ',';
            return s;
        }

        const Range* range (const ReferenceProfile& p, const char* key)
        {
            const auto it = p.ranges.find (key);
            return it != p.ranges.end() && it->second.valid ? &it->second : nullptr;
        }

        Finding make (const char* id, const char* title, Severity s, const std::string& text, const std::string& fix,
                      const std::string& measured, double prio, const std::string& note)
        {
            Finding f;
            f.id = id; f.title = title; f.severity = s;
            f.text = de (text); f.fix = de (fix); f.measured = de (measured);
            f.priority = prio; f.note = note;
            return f;
        }

        // Umrechnung "Silben-Dynamik zu viel" -> Gain Reduction auf den lauten Silben.
        // Kompressor-Sweep (Ratio 4:1) auf zwei Rohspuren: die ersten ~3 dB GR senken die Silben-Dynamik
        // etwa 1:1, danach flacht es ab (alles liegt über der Schwelle, nur die Ratio zählt).
        // Deshalb ab ~4 dB Abstand: schnellere Attack / höhere Ratio oder zweistufig.
        double gainReductionFor (double excessDb) { return std::clamp (roundHalf (excessDb * 1.2), 1.0, 12.0); }
    }

    VocalDynamics measureVocalDynamics (const AudioBuffer& a)
    {
        VocalDynamics d;
        d.gapDepthDb = wordGapDepthDb (a);
        if (a.channels.empty()) return d;
        const std::size_t w = (std::size_t) (a.sampleRate * 0.05);
        if (w == 0) return d;
        std::vector<double> lv;
        for (std::size_t i = 0; i + w <= a.numSamples(); i += w)
        {
            double p = 0;
            for (const auto& ch : a.channels)
                for (std::size_t j = i; j < i + w; ++j) p += (double) ch[j] * ch[j];
            lv.push_back (toDb (p / (double) (w * a.channels.size())));
        }
        if (lv.size() < 40) return d;
        const double top = percentile (lv, 95);
        std::vector<double> act;
        for (double v : lv) if (v > top - 25.0) act.push_back (v);
        if (act.size() < 20) return d;
        d.syllableDb = percentile (act, 90) - percentile (act, 30);
        d.syllablePeakDb = percentile (act, 95) - percentile (act, 50);
        return d;
    }

    std::vector<Finding> evaluateVocalAgainstProfile (const TrackAnalysis& t, const VocalDynamics& d,
                                                      double sideToMid, const ReferenceProfile& p, bool raw)
    {
        std::vector<Finding> out;
        if (p.empty()) return out;
        const std::string src = "gegen deine Referenzen (Vocal-Stems)";

        // ---------------- K01/K02 Kompression
        if (const auto* r = range (p, "Silben-Dynamik"); r != nullptr && d.syllableDb > 0)
        {
            const double v = d.syllableDb;
            const std::string measured = fmt2 ("%.1f dB (Ziel %.1f dB)", v, r->p50);
            if (raw)
            {
                const double excess = v - r->p50;
                if (excess > 1.0)
                {
                    const double gr = gainReductionFor (excess);
                    std::string fix = fmt ("Kompressor ca. %.1f dB Gain Reduction auf den lauten Silben, Ratio 4:1, Attack 5–10 ms, Release 50–80 ms", gr);
                    if (excess > 4.0)
                        fix = fmt2 ("Zweistufig: erst schneller Kompressor (Attack 1 ms, Ratio 8:1) ca. %.1f dB nur auf Spitzen, dann Kompressor 4:1 ca. %.1f dB", roundHalf (gr * 0.4), roundHalf (gr * 0.6));
                    out.push_back (make ("K01", "Kompression bis zum Ziel", excess > 2.0 ? Severity::warning : Severity::info,
                                         fmt ("Deine Silben schwanken %.1f dB mehr als bei deinen Referenzen", excess), fix,
                                         measured, excess, src));
                }
                else
                    out.push_back (make ("K01", "Kompression bis zum Ziel", Severity::ok,
                                         "Dynamik schon nah am Ziel, nur leicht komprimieren (1–2 dB)", "", measured, 0, src));
            }
            else if (v > r->p90)
            {
                const double excess = v - r->p50;
                out.push_back (make ("K01", "Zu wenig Kompression", v > r->p90 + 2.0 ? Severity::problem : Severity::warning,
                                     fmt ("Silben schwanken %.1f dB mehr als im Median deiner Referenzen", excess),
                                     fmt ("Ca. %.1f dB mehr Gain Reduction (Ratio 4:1, Attack 5–10 ms) oder Clip-Gain auf die lauten Wörter", gainReductionFor (excess)),
                                     measured, excess, src));
            }
            else if (v < r->p10)
            {
                const double lack = r->p50 - v;
                out.push_back (make ("K02", "Überkomprimiert", v < r->p10 - 1.5 ? Severity::problem : Severity::warning,
                                     fmt ("Silben schwanken %.1f dB weniger als im Median deiner Referenzen, klingt platt", lack),
                                     fmt ("Gain Reduction um ca. %.1f dB verringern oder langsamere Attack (15–30 ms)", gainReductionFor (lack)),
                                     measured, lack, src));
            }
            else
                out.push_back (make ("K01", "Kompression", Severity::ok, "Dynamik im Bereich deiner Referenzen", "", measured, 0, src));
        }

        // ---------------- T13 Klangkurve pro Oktave, S01 Low Cut
        const auto bal = octaveBalance (t, p.octaveCentres);
        struct Move { double c, db, dev; };
        std::vector<Move> moves;
        for (std::size_t i = 0; i < bal.size() && i < p.octaves.size(); ++i)
        {
            const auto& r = p.octaves[i];
            const double c = p.octaveCentres[i];
            if (! r.valid || bal[i] < -140 || c > t.sampleRate * 0.45) continue;
            if (c < 100)
            {
                // Bassbereich: bei Vocals Low Cut statt Glocken-EQ
                if (c > 50 && bal[i] > r.p90 + 3.0)
                    out.push_back (make ("S01", "Low Cut", raw ? Severity::warning : Severity::problem,
                                         fmt ("Unter 100 Hz %.0f dB mehr als bei deinen Referenzen", bal[i] - r.p50),
                                         "Low Cut 80–100 Hz, 12–18 dB/Okt.", fmt ("%.1f dB", bal[i]), bal[i] - r.p90, src));
                continue;
            }
            if (c > 12000 && t.lossySource) continue;
            if (raw)
            {
                // Rohspur: Weg zum Median, nur spürbare Schritte
                const double dev = bal[i] - r.p50;
                if (std::fabs (dev) >= 2.0) moves.push_back ({ c, -dev, std::fabs (dev) });
            }
            else if (bal[i] > r.p90 + 1.0) moves.push_back ({ c, -(bal[i] - r.p50), bal[i] - r.p90 });
            else if (bal[i] < r.p10 - 1.0) moves.push_back ({ c, r.p50 - bal[i], r.p10 - bal[i] });
        }
        // Größte Abweichungen zuerst, höchstens 4 Bänder (sonst wird es ein Grafik-EQ)
        std::sort (moves.begin(), moves.end(), [] (const Move& a, const Move& b) { return a.dev > b.dev; });
        if (moves.size() > 4) moves.resize (4);
        for (const auto& m : moves)
        {
            // 0,7: eine Oktave anheben verschiebt auch die Normierung der anderen
            const double gain = std::clamp (roundHalf (m.db * 0.7), -6.0, 6.0);
            if (std::fabs (gain) < 1.0) continue;
            const bool more = m.db > 0;
            auto f = make ("T13", "Klangkurve", raw ? Severity::info : (m.dev > 3.0 ? Severity::problem : Severity::warning),
                           (more ? "Zu wenig um " : "Zu viel um ") + hzLabel (m.c) + fmt (" (%.1f dB neben dem Median deiner Referenzen)", std::fabs (m.db)),
                           fmt ("EQ %+.1f dB bei ", gain) + hzLabel (m.c) + ", Glocke Q 1",
                           fmt ("%+.1f dB", -m.db), m.dev, raw ? src + ", Rohspur: Mikro und Stimme verschieben das" : src);
            f.frequency = m.c;
            out.push_back (std::move (f));
        }

        // ---------------- T06/T07 Zischlaute
        if (const auto* r = range (p, "Zischlaute (S zu Vokal)"); r != nullptr && t.sibilantToVowelDb > -149.0)
        {
            const double v = t.sibilantToVowelDb;
            const std::string measured = fmt2 ("%.1f dB (Ziel %.1f dB)", v, r->p50);
            Finding f;
            bool any = true;
            if (v > (raw ? r->p50 + 2.0 : r->p90))
                f = make ("T06", "Zischlaute", raw ? Severity::info : Severity::warning,
                          fmt ("S-Laute %.1f dB lauter als im Median deiner Referenzen", v - r->p50),
                          fmt ("De-Esser ca. %.1f dB Reduktion", std::clamp (roundHalf (v - r->p50), 1.0, 10.0))
                              + (t.sibilantFrequency > 0 ? " um " + hzLabel (std::round (t.sibilantFrequency / 100.0) * 100.0) : std::string()),
                          measured, v - r->p50, src);
            else if (! raw && v < r->p10)
                f = make ("T07", "Zischlaute zu stark entfernt", Severity::warning,
                          fmt ("S-Laute %.1f dB leiser als im Median deiner Referenzen, klingt gelispelt", r->p50 - v),
                          "De-Esser zurücknehmen", measured, r->p50 - v, src);
            else
                any = false;
            if (any)
            {
                f.frequency = t.sibilantFrequency;
                out.push_back (std::move (f));
            }
        }

        if (! raw)
        {
            // ---------------- M15 Lücken-Füllung (Hall/Delay)
            if (const auto* r = range (p, "Abstand Wort zu Lücke"); r != nullptr && d.gapDepthDb > 0)
            {
                const double v = d.gapDepthDb;
                const std::string measured = fmt2 ("%.1f dB (Ziel %.1f dB)", v, r->p50);
                if (v < r->p10)
                    out.push_back (make ("M15", "Lücken zu voll", Severity::warning,
                                         fmt ("Zwischen den Wörtern bleibt %.1f dB mehr stehen als bei deinen Referenzen", r->p50 - v),
                                         "Hall/Delay-Send leiser oder kürzer, Ducking auf den Effekt-Return", measured, r->p50 - v, src));
                else if (v > r->p90)
                    out.push_back (make ("M15", "Sehr trocken", Severity::info,
                                         fmt ("Lücken %.1f dB leerer als bei deinen Referenzen", v - r->p50),
                                         "Etwas Hall/Delay als Send (Return −20 bis −15 dB unter dem Lead) oder Gate/Expander lockern", measured, (v - r->p50) * 0.5, src));
            }

            // ---------------- M07 Breite
            if (const auto* r = range (p, "Breite (Seite zu Mitte)"); r != nullptr && sideToMid > -100.0 && sideToMid > r->p90 + 2.0)
                out.push_back (make ("M07", "Vocal breit", Severity::warning,
                                     fmt ("Seitenanteil %.0f dB über deinen Referenzen", sideToMid - r->p90),
                                     "Stereo-Hall/Doubler zurücknehmen, Lead mono lassen", fmt ("%.1f dB", sideToMid), sideToMid - r->p90, src));
        }
        return out;
    }

    std::vector<Finding> evaluateVocalWithProfile (const TrackAnalysis& t, const VocalDynamics& d,
                                                   double sideToMid, const ReferenceProfile& p, bool raw)
    {
        auto base = evaluateVocal (t, raw);
        if (p.empty()) return base;
        auto extra = evaluateVocalAgainstProfile (t, d, sideToMid, p, raw);
        const auto replaced = [] (const std::string& id) { return id == "K01" || id == "K02" || id == "T01" || id == "T06" || id == "T07"; };
        base.erase (std::remove_if (base.begin(), base.end(), [&] (const Finding& f) { return replaced (f.id); }), base.end());
        // S01 aus dem Profil nur, wenn der feste Rumpel-Check nicht schon anschlägt
        const bool rumbleFlagged = std::any_of (base.begin(), base.end(), [] (const Finding& f) { return f.id == "S01" && f.severity != Severity::ok; });
        for (auto& f : extra)
            if (! (f.id == "S01" && rumbleFlagged)) base.push_back (std::move (f));
        return base;
    }
}
