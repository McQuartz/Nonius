#include "nonius/profile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace nonius
{
    namespace
    {
        std::string fmt (const char* f, double v)
        {
            char buf[96];
            std::snprintf (buf, sizeof (buf), f, v);
            return buf;
        }

        std::string hzLabel (double f)
        {
            return f >= 1000 ? fmt ("%.0f kHz", f / 1000.0) : fmt ("%.0f Hz", f);
        }

        double roundHalf (double v) { return std::round (v * 2.0) / 2.0; }

        double totalPower (const TrackAnalysis& t)
        {
            double total = 0;
            for (double l : t.spectrum.levelDb) total += std::pow (10.0, l / 10.0);
            return total;
        }
    }

    int stageOf (const std::string& id)
    {
        if (id.empty()) return 0;
        switch (id[0])
        {
            case 'A': return 1;
            case 'S': return 2;
            case 'K': return 3;
            case 'T': return 4;
            case 'M': return 5;
            default:  return 0;
        }
    }

    std::vector<double> octaveBalance (const TrackAnalysis& t, const std::vector<double>& centres)
    {
        std::vector<double> out;
        const double total = totalPower (t);
        const auto& bands = t.spectrum.bands;
        for (double c : centres)
        {
            const double lo = c / std::sqrt (2.0), hi = c * std::sqrt (2.0);
            double p = 0;
            for (std::size_t i = 0; i < bands.size() && i < t.spectrum.levelDb.size(); ++i)
                if (bands[i].centre >= lo && bands[i].centre < hi) p += std::pow (10.0, t.spectrum.levelDb[i] / 10.0);
            out.push_back (total > 0 ? toDb (p / total) : -150.0);
        }
        return out;
    }

    FourBands fourBandBalance (const TrackAnalysis& t)
    {
        FourBands r;
        const double total = totalPower (t);
        if (total <= 0) return r;
        double low = 0, lm = 0, hm = 0, hi = 0;
        for (std::size_t i = 0; i < t.spectrum.bands.size() && i < t.spectrum.levelDb.size(); ++i)
        {
            const double p = std::pow (10.0, t.spectrum.levelDb[i] / 10.0), f = t.spectrum.bands[i].centre;
            if (f < 250) low += p; else if (f < 2000) lm += p; else if (f < 8000) hm += p; else hi += p;
        }
        r.low = toDb (low / total); r.lowMid = toDb (lm / total); r.highMid = toDb (hm / total); r.high = toDb (hi / total);
        return r;
    }

    double sideToMidDb (const AudioBuffer& a)
    {
        if (a.channels.size() < 2) return -150.0;
        double mm = 0, ss = 0;
        for (std::size_t i = 0; i < a.numSamples(); ++i)
        {
            const double m = 0.5 * (a.channels[0][i] + a.channels[1][i]), s = 0.5 * (a.channels[0][i] - a.channels[1][i]);
            mm += m * m; ss += s * s;
        }
        return mm > 0 ? toDb (ss / mm) : -150.0;
    }

    std::vector<Finding> evaluateAgainstProfile (const TrackAnalysis& t, double sideToMid, const ReferenceProfile& p, bool isMaster)
    {
        std::vector<Finding> out;
        if (p.empty()) return out;

        auto add = [&] (const char* id, const char* title, Severity s, std::string text, std::string fix, std::string measured, double prio)
        {
            Finding f;
            f.id = id; f.title = title; f.severity = s; f.text = std::move (text); f.fix = std::move (fix);
            f.measured = std::move (measured); f.priority = prio; f.note = "gegen deine Referenzen";
            out.push_back (std::move (f));
            return &out.back();
        };

        // M14 Klangbalance pro Oktave: außerhalb P10–P90 um mehr als 1,5 dB
        const auto bal = octaveBalance (t, p.octaveCentres);
        for (std::size_t i = 0; i < bal.size() && i < p.octaves.size(); ++i)
        {
            const auto& r = p.octaves[i];
            if (! r.valid) continue;
            const double c = p.octaveCentres[i];
            if (c > t.sampleRate * 0.45) continue;
            if (bal[i] > r.p90 + 1.5)
            {
                const double d = bal[i] - r.p50;
                add ("M14", "Klangbalance", d > 4.0 ? Severity::problem : Severity::warning,
                     "Zu viel um " + hzLabel (c) + fmt (" (%.1f dB über dem Median deiner Referenzen)", d),
                     "Breiter EQ " + fmt ("−%.1f dB bei ", std::clamp (roundHalf (d * 0.7), 1.0, 6.0)) + hzLabel (c) + ", Q 0,7",
                     fmt ("%.1f dB", bal[i]), d)->frequency = c;
            }
            else if (bal[i] < r.p10 - 1.5)
            {
                const double d = r.p50 - bal[i];
                add ("M14", "Klangbalance", d > 4.0 ? Severity::problem : Severity::warning,
                     "Zu wenig um " + hzLabel (c) + fmt (" (%.1f dB unter dem Median deiner Referenzen)", d),
                     "Breiter EQ " + fmt ("+%.1f dB bei ", std::clamp (roundHalf (d * 0.7), 1.0, 6.0)) + hzLabel (c) + ", Q 0,7",
                     fmt ("%.1f dB", bal[i]), d)->frequency = c;
            }
        }

        // T01/T02 Neigung
        if (auto it = p.ranges.find ("Spektrale Neigung"); it != p.ranges.end() && it->second.valid)
        {
            const auto& r = it->second;
            if (t.spectrum.tiltDbPerOct < r.p10 - 0.3)
                add ("T01", "Zu dunkel", Severity::warning, fmt ("Insgesamt dunkler als deine Referenzen (%.1f dB/Okt.)", t.spectrum.tiltDbPerOct),
                     fmt ("High Shelf +%.1f dB ab 5 kHz", std::clamp (roundHalf ((r.p50 - t.spectrum.tiltDbPerOct) * 4.0), 1.0, 4.0)),
                     fmt ("%.1f dB/Okt.", t.spectrum.tiltDbPerOct), (r.p10 - t.spectrum.tiltDbPerOct) * 4.0);
            else if (t.spectrum.tiltDbPerOct > r.p90 + 0.3)
                add ("T02", "Zu hell", Severity::warning, fmt ("Insgesamt heller als deine Referenzen (%.1f dB/Okt.)", t.spectrum.tiltDbPerOct),
                     fmt ("High Shelf −%.1f dB ab 5 kHz", std::clamp (roundHalf ((t.spectrum.tiltDbPerOct - r.p50) * 4.0), 1.0, 4.0)),
                     fmt ("%.1f dB/Okt.", t.spectrum.tiltDbPerOct), (t.spectrum.tiltDbPerOct - r.p90) * 4.0);
        }

        // Breite
        if (auto it = p.ranges.find ("Breite (Seite zu Mitte)"); it != p.ranges.end() && it->second.valid && sideToMid > -100.0)
        {
            const auto& r = it->second;
            if (sideToMid < r.p10 - 2.0)
                add ("M07", "Breite", Severity::info, fmt ("Schmaler als deine Referenzen (Seite %.0f dB unter Mitte)", -sideToMid),
                     "Info: Doubles/Adlibs/Effekte breiter verteilen, falls gewollt", fmt ("%.1f dB", sideToMid), 0.5);
            else if (sideToMid > r.p90 + 2.0)
                add ("M07", "Breite", Severity::warning, fmt ("Breiter als deine Referenzen (Seite %.0f dB unter Mitte)", -sideToMid),
                     "Stereo-Verbreiterung zurücknehmen, Mono-Kompatibilität prüfen", fmt ("%.1f dB", sideToMid), sideToMid - r.p90);
        }

        // M13 Lautheit (nur Info, nur auf dem Master sinnvoll)
        if (isMaster)
            if (auto it = p.ranges.find ("Lautheit integriert"); it != p.ranges.end() && it->second.valid)
            {
                const double d = t.loudness.integratedLufs - it->second.p50;
                if (std::fabs (d) > 2.0)
                    add ("M13", "Lautheit", Severity::info, fmt ("Mix %.1f LU ", std::fabs (d)) + (d < 0 ? "leiser" : "lauter") + " als deine Referenzen",
                         "Info, kein Fehler: fürs Mastering", fmt ("%.1f LUFS", t.loudness.integratedLufs), 0.2);
            }

        return out;
    }
}
