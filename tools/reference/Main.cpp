// nonius-referenz: misst einen Ordner mit Referenz-Songs (komplette Mixe) und baut daraus das
// "Standard"-Profil (P10/P50/P90 je Messwert). Optional werden eigene Mixe dagegen verglichen.
//
// Aufruf:
//   nonius-referenz --ordner REFERENZEN [--favoriten "teil1;teil2"] [--nur-vergleich "teil"]
//                   [--mix "Name=vocal.wav+beat.wav" ...] [--out bericht.md] [--profil profil.json]
// Favoriten zählen doppelt, "nur Vergleich" wird gemessen, aber nicht ins Profil gerechnet.
#include <juce_audio_formats/juce_audio_formats.h>

#include "nonius/analysis.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

using namespace nonius;

namespace
{
    struct MixStats
    {
        juce::String name;
        double weight = 1.0;
        double lufs = 0, lra = 0, truePeak = 0, crest100 = 0, crest1s = 0, tilt = 0;
        double low = 0, lowMid = 0, highMid = 0, high = 0; // 4 Bänder relativ zur Gesamtleistung (dB)
        double sideToMid = 0, bassSide = 0, correlation = 0;
        std::vector<double> octaves;                         // Oktavspektrum, normiert
    };

    const std::vector<Band>& octaveBands()
    {
        static const auto b = fractionalOctaveBands (1, 31.5, 16000.0);
        return b;
    }

    bool load (const juce::File& f, AudioBuffer& a)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr) return false;
        const int len = (int) r->lengthInSamples;
        const int chs = (int) std::min<unsigned int> (2, r->numChannels);
        juce::AudioBuffer<float> b (chs, len);
        r->read (&b, 0, len, 0, true, chs > 1);
        a.sampleRate = r->sampleRate;
        a.lossySource = ! f.hasFileExtension ("wav;aif;aiff;flac");
        a.channels.clear();
        for (int c = 0; c < chs; ++c)
            a.channels.emplace_back (b.getReadPointer (c), b.getReadPointer (c) + len);
        return true;
    }

    MixStats measure (const AudioBuffer& a, const juce::String& name)
    {
        MixStats m;
        m.name = name;
        auto t = analyseTrack (a, name.toStdString(), Role::sum);
        m.lufs = t.loudness.integratedLufs;
        m.lra = t.loudness.lra;
        m.truePeak = t.truePeakDb;
        m.crest100 = t.crest100msDb;
        m.crest1s = t.crest1sDb;
        m.tilt = t.spectrum.tiltDbPerOct;
        m.bassSide = t.sideBelow120Db;
        m.correlation = t.correlation;

        // Bänder relativ zur Summe aller Bänder
        double total = 0, low = 0, lm = 0, hm = 0, hi = 0;
        const auto& bands = t.spectrum.bands;
        for (std::size_t i = 0; i < bands.size(); ++i)
        {
            const double p = std::pow (10.0, t.spectrum.levelDb[i] / 10.0);
            total += p;
            const double f = bands[i].centre;
            if (f < 250) low += p; else if (f < 2000) lm += p; else if (f < 8000) hm += p; else hi += p;
        }
        m.low = toDb (low / total); m.lowMid = toDb (lm / total); m.highMid = toDb (hm / total); m.high = toDb (hi / total);

        for (const auto& o : octaveBands())
        {
            double p = 0;
            for (std::size_t i = 0; i < bands.size(); ++i)
                if (bands[i].centre >= o.low && bands[i].centre < o.high) p += std::pow (10.0, t.spectrum.levelDb[i] / 10.0);
            m.octaves.push_back (toDb (p / total));
        }

        // Breite: Seite zu Mitte gesamt
        if (a.channels.size() >= 2)
        {
            double mm = 0, ss = 0;
            for (std::size_t i = 0; i < a.numSamples(); ++i)
            {
                const double mv = 0.5 * (a.channels[0][i] + a.channels[1][i]), sv = 0.5 * (a.channels[0][i] - a.channels[1][i]);
                mm += mv * mv; ss += sv * sv;
            }
            m.sideToMid = toDb (ss / std::max (mm, 1e-30));
        }
        else m.sideToMid = -150;
        return m;
    }

    // Gewichtetes Perzentil (Gewicht = Anzahl Wiederholungen)
    double wPercentile (const std::vector<MixStats>& v, double MixStats::* field, double p)
    {
        std::vector<double> x;
        for (const auto& s : v)
            for (int k = 0; k < (int) std::round (s.weight); ++k) x.push_back (s.*field);
        return percentile (x, p);
    }

    double wPercentileOct (const std::vector<MixStats>& v, std::size_t band, double p)
    {
        std::vector<double> x;
        for (const auto& s : v)
            for (int k = 0; k < (int) std::round (s.weight); ++k) x.push_back (s.octaves[band]);
        return percentile (x, p);
    }

    juce::String n1 (double v, int d = 1) { return juce::String (v, d); }

    bool matchesAny (const juce::String& name, const juce::String& list)
    {
        for (auto& part : juce::StringArray::fromTokens (list, ";", ""))
            if (part.trim().isNotEmpty() && name.containsIgnoreCase (part.trim())) return true;
        return false;
    }
}

int main (int argc, char* argv[])
{
    std::map<juce::String, juce::String> args;
    juce::StringArray mixes;
    for (int i = 1; i + 1 < argc; i += 2)
    {
        const auto key = juce::String (argv[i]).trimCharactersAtStart ("-");
        const auto val = juce::String::fromUTF8 (argv[i + 1]);
        if (key == "mix") mixes.add (val); else args[key] = val;
    }
    if (! args.count ("ordner")) { std::printf ("Aufruf: nonius-referenz --ordner DIR [--favoriten a;b] [--nur-vergleich c] [--mix Name=v.wav+b.wav] [--out x.md] [--profil p.json]\n"); return 1; }

    const juce::String favs = args.count ("favoriten") ? args["favoriten"] : juce::String();
    const juce::String onlyCompare = args.count ("nur-vergleich") ? args["nur-vergleich"] : juce::String();

    std::vector<MixStats> profile, comparison;
    auto files = juce::File (args["ordner"]).findChildFiles (juce::File::findFiles, false, "*.mp3;*.wav;*.flac;*.aif;*.aiff;*.ogg");
    files.sort();
    for (const auto& f : files)
    {
        AudioBuffer a;
        if (! load (f, a)) { std::fprintf (stderr, "übersprungen: %s\n", f.getFileName().toRawUTF8()); continue; }
        std::printf ("messe %s\n", f.getFileName().toRawUTF8());
        auto m = measure (a, f.getFileNameWithoutExtension());
        if (matchesAny (m.name, onlyCompare)) { m.weight = 0; comparison.push_back (m); continue; }
        m.weight = matchesAny (m.name, favs) ? 2.0 : 1.0;
        profile.push_back (m);
    }
    if (profile.empty()) { std::printf ("Keine lesbaren Referenzen\n"); return 2; }

    // Eigene Mixe: Vocal + Beat summiert (ohne Master)
    std::vector<MixStats> own;
    for (const auto& spec : mixes)
    {
        const auto name = spec.upToFirstOccurrenceOf ("=", false, false);
        auto parts = juce::StringArray::fromTokens (spec.fromFirstOccurrenceOf ("=", false, false), "+", "");
        AudioBuffer sum;
        bool ok = true;
        for (const auto& p : parts)
        {
            AudioBuffer a;
            if (! load (juce::File (p.trim()), a)) { ok = false; break; }
            if (sum.channels.empty()) { sum = a; if (sum.channels.size() == 1) sum.channels.push_back (sum.channels[0]); continue; }
            if (a.sampleRate != sum.sampleRate) { ok = false; break; }
            const std::size_t len = std::min (sum.numSamples(), a.numSamples());
            for (auto& ch : sum.channels) ch.resize (len);
            for (std::size_t c = 0; c < 2; ++c)
                for (std::size_t i = 0; i < len; ++i)
                    sum.channels[c][i] += a.channels[std::min (c, a.channels.size() - 1)][i];
        }
        if (ok && ! sum.channels.empty()) own.push_back (measure (sum, name));
        else std::fprintf (stderr, "Mix %s nicht lesbar oder verschiedene Sampleraten\n", name.toRawUTF8());
    }

    struct Field { const char* label; double MixStats::* f; const char* unit; };
    const std::vector<Field> fields = {
        { "Lautheit integriert", &MixStats::lufs, "LUFS" },
        { "Loudness Range", &MixStats::lra, "LU" },
        { "True Peak", &MixStats::truePeak, "dBTP" },
        { "Crest 100 ms", &MixStats::crest100, "dB" },
        { "Crest 1 s", &MixStats::crest1s, "dB" },
        { "Spektrale Neigung", &MixStats::tilt, "dB/Okt." },
        { "Bass < 250 Hz", &MixStats::low, "dB" },
        { "Tiefe Mitten 250 Hz–2 kHz", &MixStats::lowMid, "dB" },
        { "Hohe Mitten 2–8 kHz", &MixStats::highMid, "dB" },
        { "Höhen > 8 kHz", &MixStats::high, "dB" },
        { "Breite (Seite zu Mitte)", &MixStats::sideToMid, "dB" },
        { "Bass-Seite < 120 Hz", &MixStats::bassSide, "dB" },
    };

    juce::String md;
    double totalWeight = 0;
    for (auto& p : profile) totalWeight += p.weight;
    md << "# NONIUS Standard-Profil aus deinen Referenzen\n\n"
       << profile.size() << " Songs im Profil (Favoriten zählen doppelt), " << comparison.size() << " nur zum Vergleich. "
       << "Gemessen werden komplette Mixe. Werte der Klangbalance sind relativ zur Gesamtleistung, also unabhängig von der Lautstärke.\n\n";

    md << "## Zielbereiche (P10 / Median / P90)\n\n| Messwert | P10 | Median | P90 |";
    for (auto& o : own) md << " " << o.name << " |";
    for (auto& c : comparison) md << " " << c.name.substring (0, 18) << " (Vergleich) |";
    md << "\n|---|---|---|---|";
    for (std::size_t i = 0; i < own.size() + comparison.size(); ++i) md << "---|";
    md << "\n";
    for (const auto& f : fields)
    {
        const double p10 = wPercentile (profile, f.f, 10), p50 = wPercentile (profile, f.f, 50), p90 = wPercentile (profile, f.f, 90);
        md << "| " << juce::String::fromUTF8 (f.label) << " (" << juce::String::fromUTF8 (f.unit) << ") | " << n1 (p10) << " | " << n1 (p50) << " | " << n1 (p90) << " |";
        for (auto& o : own)
        {
            const double v = o.*(f.f);
            md << " " << n1 (v) << (v < p10 ? juce::String::fromUTF8 (" ↓") : v > p90 ? juce::String::fromUTF8 (" ↑") : juce::String()) << " |";
        }
        for (auto& c : comparison) md << " " << n1 (c.*(f.f)) << " |";
        md << "\n";
    }
    md << "\n_↓/↑ = außerhalb des Bereichs P10–P90. Eigene Mixe sind Vocal + Beat ohne Mastering, deshalb sind Lautheit und True Peak dort nicht vergleichbar._\n\n";

    md << "## Klangbalance in Oktaven (dB relativ zur Gesamtleistung)\n\n| Band |";
    for (const auto& o : octaveBands()) md << " " << (o.centre >= 1000 ? n1 (o.centre / 1000.0, 0) + "k" : juce::String ((int) std::round (o.centre))) << " |";
    md << "\n|---|";
    for (std::size_t i = 0; i < octaveBands().size(); ++i) md << "---|";
    md << "\n";
    for (double p : { 10.0, 50.0, 90.0 })
    {
        md << "| Referenz P" << (int) p << " |";
        for (std::size_t b = 0; b < octaveBands().size(); ++b) md << " " << n1 (wPercentileOct (profile, b, p)) << " |";
        md << "\n";
    }
    for (auto& o : own)
    {
        md << "| **" << o.name << "** |";
        for (std::size_t b = 0; b < octaveBands().size(); ++b)
        {
            const double v = o.octaves[b];
            md << " " << n1 (v) << (v < wPercentileOct (profile, b, 10) ? juce::String::fromUTF8 (" ↓") : v > wPercentileOct (profile, b, 90) ? juce::String::fromUTF8 (" ↑") : juce::String()) << " |";
        }
        md << "\n";
    }
    md << "\n";

    md << "## Alle Songs\n\n| Song | Gewicht | LUFS | LRA | True Peak | Crest 1 s | Neigung | Bass | Tiefe Mitten | Hohe Mitten | Höhen | Breite | Bass-Seite |\n|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    auto row = [&] (const MixStats& s, const juce::String& w)
    {
        md << "| " << s.name << " | " << w << " | " << n1 (s.lufs) << " | " << n1 (s.lra) << " | " << n1 (s.truePeak) << " | " << n1 (s.crest1s) << " | "
           << n1 (s.tilt) << " | " << n1 (s.low) << " | " << n1 (s.lowMid) << " | " << n1 (s.highMid) << " | " << n1 (s.high) << " | " << n1 (s.sideToMid) << " | " << n1 (s.bassSide) << " |\n";
    };
    for (auto& s : profile) row (s, s.weight > 1.5 ? "Favorit" : "1");
    for (auto& s : comparison) row (s, "nur Vergleich");
    md << "\n";

    const juce::File out (args.count ("out") ? args["out"] : juce::String ("nonius-referenzen.md"));
    out.replaceWithText (md, false, false, "\n");

    if (args.count ("profil"))
    {
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty ("version", 1);
        root->setProperty ("songs", (int) profile.size());
        juce::DynamicObject::Ptr ranges = new juce::DynamicObject();
        for (const auto& f : fields)
        {
            juce::Array<juce::var> r { wPercentile (profile, f.f, 10), wPercentile (profile, f.f, 50), wPercentile (profile, f.f, 90) };
            ranges->setProperty (juce::String::fromUTF8 (f.label), r);
        }
        root->setProperty ("bereiche", ranges.get());
        juce::Array<juce::var> oct;
        for (std::size_t b = 0; b < octaveBands().size(); ++b)
        {
            juce::Array<juce::var> r { octaveBands()[b].centre, wPercentileOct (profile, b, 10), wPercentileOct (profile, b, 50), wPercentileOct (profile, b, 90) };
            oct.add (r);
        }
        root->setProperty ("oktaven", oct);
        juce::File (args["profil"]).replaceWithText (juce::JSON::toString (juce::var (root.get())), false, false, "\n");
    }

    std::printf ("Bericht: %s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
