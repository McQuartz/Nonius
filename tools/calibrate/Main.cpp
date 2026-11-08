// nonius-kalibrieren: baut bekannte Fehler in eine saubere Vocal-Spur ein und prüft,
// ob der Mess-Kern sie findet (Trefferquote, Fehlalarme, Messfehler).
//
// Aufruf: nonius-kalibrieren --vocal trockene_vocal.wav [--start 15] [--length 90] [--out kalibrierung.md]
#include <juce_audio_formats/juce_audio_formats.h>

#include "nonius/analysis.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <cstdlib>
#include <random>

using namespace nonius;

namespace
{
    AudioBuffer load (const juce::File& f, double start, double length)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        AudioBuffer a;
        if (r == nullptr) return a;
        a.sampleRate = r->sampleRate;
        const auto s0 = (juce::int64) (start * r->sampleRate);
        const int len = (int) std::min<juce::int64> ((juce::int64) (length * r->sampleRate), r->lengthInSamples - s0);
        juce::AudioBuffer<float> buf (1, len);
        r->read (&buf, 0, len, s0, true, false);
        a.channels = { std::vector<float> (buf.getReadPointer (0), buf.getReadPointer (0) + len) };
        return a;
    }

    void highPass (AudioBuffer& a, double f)
    {
        for (auto& ch : a.channels)
        {
            Biquad h1 = Biquad::highPass (a.sampleRate, f, 0.7071), h2 = h1;
            for (auto& v : ch) v = (float) h2.process (h1.process (v));
        }
    }

    void scaleToPeak (AudioBuffer& a, double peakDb)
    {
        double pk = 0;
        for (float v : a.channels[0]) pk = std::max (pk, (double) std::fabs (v));
        const double g = std::pow (10.0, peakDb / 20.0) / std::max (pk, 1e-9);
        for (auto& v : a.channels[0]) v = (float) (v * g);
    }

    // Aktive Zeitpunkte (Wortanfänge): Frames, die von leise nach laut springen
    std::vector<double> onsets (const AudioBuffer& a)
    {
        const auto fl = (std::size_t) (a.sampleRate * 0.02);
        std::vector<double> lv;
        const auto& x = a.channels[0];
        for (std::size_t s = 0; s + fl <= x.size(); s += fl)
        {
            double p = 0; for (std::size_t i = s; i < s + fl; ++i) p += (double) x[i] * x[i];
            lv.push_back (toDb (p / fl));
        }
        const double top = percentile (lv, 95);
        std::vector<double> t;
        for (std::size_t i = 5; i < lv.size(); ++i)
            if (lv[i] > top - 20 && lv[i - 3] < top - 35 && (t.empty() || i * 0.02 - t.back() > 0.8))
                t.push_back ((double) i * 0.02);
        return t;
    }

    // FFT-Faltung (Overlap-Add)
    std::vector<float> convolve (const std::vector<float>& x, const std::vector<float>& h)
    {
        std::size_t N = 1;
        while (N < h.size() * 2) N <<= 1;
        const std::size_t block = N - h.size() + 1;
        std::vector<std::complex<double>> H (N);
        for (std::size_t i = 0; i < h.size(); ++i) H[i] = h[i];
        fft (H);
        std::vector<float> y (x.size() + h.size(), 0.0f);
        for (std::size_t s = 0; s < x.size(); s += block)
        {
            std::vector<std::complex<double>> X (N);
            for (std::size_t i = 0; i < block && s + i < x.size(); ++i) X[i] = x[s + i];
            fft (X);
            for (std::size_t k = 0; k < N; ++k) X[k] = std::conj (X[k] * H[k]);
            fft (X); // inverse über konjugierte FFT
            for (std::size_t i = 0; i < N && s + i < y.size(); ++i) y[s + i] += (float) (std::conj (X[i]).real() / (double) N);
        }
        y.resize (x.size());
        return y;
    }

    struct Hits { int truePos = 0, falsePos = 0, missed = 0; };

    Hits matchEvents (const std::vector<double>& detected, const std::vector<double>& truth, double tol)
    {
        Hits h;
        std::vector<bool> used (detected.size(), false);
        for (double t : truth)
        {
            bool found = false;
            for (std::size_t i = 0; i < detected.size(); ++i)
                if (! used[i] && std::fabs (detected[i] - t) <= tol) { used[i] = true; found = true; break; }
            if (found) ++h.truePos; else ++h.missed;
        }
        for (bool u : used) if (! u) ++h.falsePos;
        return h;
    }

    juce::String n1 (double v, int d = 1) { return juce::String (v, d); }
}

int main (int argc, char* argv[])
{
    std::map<juce::String, juce::String> args;
    for (int i = 1; i + 1 < argc; i += 2)
        args[juce::String (argv[i]).trimCharactersAtStart ("-")] = juce::String::fromUTF8 (argv[i + 1]);
    if (! args.count ("vocal")) { std::printf ("Aufruf: nonius-kalibrieren --vocal datei [--start s] [--length s] [--out x.md]\n"); return 1; }

    const double start = args.count ("start") ? args["start"].getDoubleValue() : 0.0;
    const double length = args.count ("length") ? args["length"].getDoubleValue() : 90.0;
    AudioBuffer base = load (juce::File (args["vocal"]), start, length);
    if (base.channels.empty()) { std::printf ("Datei nicht lesbar\n"); return 2; }

    // Saubere Basis: Low Cut 120 Hz (entfernt echte Popps und Rumpeln), Peak −8 dBFS
    highPass (base, 120.0);
    scaleToPeak (base, -8.0);
    const double sr = base.sampleRate;
    std::mt19937 rng (42);

    juce::String md;
    md << "# NONIUS Kalibrierung mit künstlichen Fehlern\n\n"
       << "Basis: `" << juce::File (args["vocal"]).getFileName() << "`, " << n1 (start, 0) << "–" << n1 (start + length, 0)
       << " s, Low Cut 120 Hz, Peak −8 dBFS. In diese Spur werden Fehler mit bekannter Größe eingebaut.\n\n";

    auto baseT = analyseTrack (base, "Basis", Role::leadVocal);

    // ------------------------------------------------------------ Popps
    {
        md << "## Popps (A07)\n\nKünstlicher Popp: Tiefpass-Stoß 30–80 Hz, 40 ms, an Wortanfängen. Gleichzeitig zählt, was ohne eingebaute Popps gemeldet wird (Fehlalarme).\n\n"
           << "| Popp-Pegel (relativ zum Wort-Peak) | eingebaut | gefunden | verpasst | Fehlalarme |\n|---|---|---|---|---|\n";
        md << "| keine (Basis) | 0 | 0 | 0 | " << baseT.plosives.count << " |\n";
        auto on = onsets (base);
        for (double relDb : { 0.0, -6.0, -12.0 })
        {
            AudioBuffer a = base;
            std::vector<double> truth;
            const double amp = std::pow (10.0, (-8.0 + relDb) / 20.0);
            for (std::size_t k = 0; k < on.size(); k += 3)
            {
                const auto s0 = (std::size_t) (on[k] * sr);
                const auto len = (std::size_t) (0.04 * sr);
                if (s0 + len >= a.channels[0].size()) break;
                const double f = 30.0 + 50.0 * std::uniform_real_distribution<double> (0, 1) (rng);
                for (std::size_t i = 0; i < len; ++i)
                {
                    const double t = (double) i / sr;
                    const double env = std::exp (-t / 0.012) * (1.0 - std::exp (-t / 0.0015));
                    a.channels[0][s0 + i] += (float) (amp * 2.5 * env * std::sin (2 * kPi * f * t));
                }
                truth.push_back (on[k]);
            }
            auto t = analyseTrack (a, "popps", Role::leadVocal);
            auto h = matchEvents (t.plosives.timesSeconds, truth, 0.1);
            if (std::getenv ("NONIUS_DEBUG"))
            {
                std::fprintf (stderr, "Popps %.0f dB\n wahr:", relDb);
                for (double x : truth) std::fprintf (stderr, " %.2f", x);
                std::fprintf (stderr, "\n gefunden:");
                for (double x : t.plosives.timesSeconds) std::fprintf (stderr, " %.2f", x);
                std::fprintf (stderr, "\n");
            }
            md << "| " << n1 (relDb, 0) << " dB | " << (int) truth.size() << " | " << h.truePos << " | " << h.missed << " | " << h.falsePos << " |\n";
        }
        md << "\n";
    }

    // ------------------------------------------------------------ Clipping
    {
        md << "## Clipping (A01)\n\nSpur um x dB verstärkt und bei 0 dBFS hart begrenzt, danach wieder auf −8 dBFS abgesenkt (wie beim Bearbeiten einer übersteuerten Aufnahme).\n\n"
           << "| Übersteuerung | Samples über 0 dBFS | gefundene Stellen |\n|---|---|---|\n";
        md << "| keine | 0 | " << baseT.clippingRuns << " |\n";
        for (double over : { 1.0, 3.0, 6.0 })
        {
            AudioBuffer a = base;
            scaleToPeak (a, over);
            int clipped = 0;
            for (auto& v : a.channels[0]) if (std::fabs (v) > 1.0f) { v = std::clamp (v, -1.0f, 1.0f); ++clipped; }
            scaleToPeak (a, -8.0);
            auto t = analyseTrack (a, "clip", Role::leadVocal);
            md << "| +" << n1 (over, 0) << " dB | " << clipped << " | " << t.clippingRuns << " |\n";
        }
        md << "\n";
    }

    // ------------------------------------------------------------ Rauschen
    {
        md << "## Rauschen (A04)\n\nWeißes Rauschen mit bekanntem Pegel addiert (A-bewertet gemessen).\n\n"
           << "| Rauschen (unbewertet RMS) | gemessen (A) | Status |\n|---|---|---|\n";
        for (double nDb : { -80.0, -65.0, -55.0, -45.0 })
        {
            AudioBuffer a = base;
            std::normal_distribution<double> d (0.0, std::pow (10.0, nDb / 20.0));
            for (auto& v : a.channels[0]) v += (float) d (rng);
            auto t = analyseTrack (a, "noise", Role::leadVocal);
            auto f = evaluateVocal (t, true);
            juce::String st = "–";
            for (auto& x : f) if (x.id == "A04") st = severityLabel (x.severity);
            md << "| " << n1 (nDb, 0) << " dBFS | " << (t.noiseFloorReliable ? n1 (t.noiseFloorDbA) + " dBFS" : juce::String ("zu wenig Pausen")) << " | " << st << " |\n";
        }
        md << "\n";
    }

    // ------------------------------------------------------------ Brummen
    {
        md << "## Brummen (A05)\n\n50 Hz + Obertöne (100, 150 Hz, je −6 dB) plus Rauschen −70 dBFS als Pausen-Untergrund.\n\n"
           << "| Brumm-Pegel | gemessen über Nachbarn | erkannt |\n|---|---|---|\n";
        for (double hDb : { -100.0, -75.0, -65.0, -55.0 })
        {
            AudioBuffer a = base;
            std::normal_distribution<double> d (0.0, std::pow (10.0, -70.0 / 20.0));
            const double amp = std::pow (10.0, hDb / 20.0);
            for (std::size_t i = 0; i < a.channels[0].size(); ++i)
            {
                const double t = (double) i / sr;
                a.channels[0][i] += (float) (d (rng) + amp * (std::sin (2 * kPi * 50 * t) + 0.5 * std::sin (2 * kPi * 100 * t) + 0.5 * std::sin (2 * kPi * 150 * t)));
            }
            auto t = analyseTrack (a, "hum", Role::leadVocal);
            md << "| " << (hDb < -99 ? juce::String ("keins") : n1 (hDb, 0) + " dBFS") << " | " << n1 (t.humExcessDb) << " dB bei " << n1 (t.humFrequency, 0) << " Hz | "
               << (t.humExcessDb >= 10.0 ? "ja" : "nein") << " |\n";
        }
        md << "\n";
    }

    // ------------------------------------------------------------ Hall
    {
        md << "## Raumhall (A06)\n\nGefaltet mit künstlicher Raumantwort (exponentiell abklingendes Rauschen, bekannte RT60), 50 % Direktschall.\n\n"
           << "| RT60 eingebaut | geschätzt | Sicherheit | Ausklänge |\n|---|---|---|---|\n";
        md << "| keiner (Basis) | " << n1 (baseT.reverbSeconds, 2) << " s | " << n1 (baseT.reverbConfidence) << " | " << baseT.reverbDecays << " |\n";
        std::fprintf (stderr, "Basis Raumanteil %.1f dB (%d)\n", baseT.roomTailDb, baseT.roomTailCount);
        for (double rt : { 0.3, 0.6, 1.0 })
        {
            const auto len = (std::size_t) (rt * 1.5 * sr);
            std::vector<float> ir (len);
            std::normal_distribution<double> d (0.0, 1.0);
            double e = 0;
            for (std::size_t i = 0; i < len; ++i) { ir[i] = (float) (d (rng) * std::pow (10.0, -3.0 * ((double) i / sr) / rt)); e += (double) ir[i] * ir[i]; }
            // Hallanteil auf gleiche Energie wie Direktschall, Direktschall vorne
            for (auto& v : ir) v = (float) (v / std::sqrt (e));
            ir[0] += 1.0f;
            AudioBuffer a = base;
            a.channels[0] = convolve (base.channels[0], ir);
            scaleToPeak (a, -8.0);
            auto t = analyseTrack (a, "rt", Role::leadVocal);
            md << "| " << n1 (rt, 1) << " s | " << n1 (t.reverbSeconds, 2) << " s | " << n1 (t.reverbConfidence) << " | " << t.reverbDecays << " |\n";
        }
        md << "\n";
    }

    // ------------------------------------------------------------ Simulierte Räume (optional)
    if (args.count ("irs"))
    {
        const juce::File dir (args["irs"]);
        auto meta = juce::JSON::parse (dir.getChildFile ("irs.json"));
        md << "## Raumhall in simulierten Räumen (A06)\n\nRaumantworten aus dem Raum-Simulator pyroomacoustics (Spiegelquellen), "
           << "Mikro nah an der Quelle wie bei einer Vocal-Aufnahme.\n\n"
           << "| Raum | Mikro-Abstand | RT60 (Simulator) | RT geschätzt | Raumanteil (60–120 ms nach Wortende) | Wortenden |\n|---|---|---|---|---|---|\n";
        if (auto* arr = meta.getArray())
            for (auto& m : *arr)
            {
                const juce::String name = m[0].toString();
                juce::AudioFormatManager fm; fm.registerBasicFormats();
                std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (dir.getChildFile (name + ".wav")));
                if (r == nullptr) continue;
                juce::AudioBuffer<float> b (1, (int) r->lengthInSamples);
                r->read (&b, 0, (int) r->lengthInSamples, 0, true, false);
                std::vector<float> ir (b.getReadPointer (0), b.getReadPointer (0) + b.getNumSamples());
                AudioBuffer a = base;
                a.channels[0] = convolve (base.channels[0], ir);
                scaleToPeak (a, -8.0);
                auto t = analyseTrack (a, "room", Role::leadVocal);
                md << "| " << name << " | " << n1 ((double) m[3] * 100.0, 0) << " cm | " << n1 ((double) m[2], 2) << " s | "
                   << n1 (t.reverbSeconds, 2) << " s (umgerechnet " << n1 (t.reverbSecondsCloseMic, 2) << " s) | " << n1 (t.roomTailDb) << " dB | " << t.roomTailCount << " |\n";
            }
        md << "\n";
    }

    // ------------------------------------------------------------ Zischlaute
    {
        md << "## Zischlaute (T06)\n\nHigh Shelf ab 5 kHz mit bekanntem Gain. Erwartung: Messwert steigt um ungefähr den Gain.\n\n"
           << "| High Shelf | S-Laute relativ zu Vokalen | Änderung | Anzahl S-Laute | Status |\n|---|---|---|---|---|\n";
        for (double g : { 0.0, 3.0, 6.0, 9.0 })
        {
            AudioBuffer a = base;
            Biquad hs = Biquad::highShelf (sr, 5000.0, g, 0.7071);
            for (auto& v : a.channels[0]) v = (float) hs.process (v);
            auto t = analyseTrack (a, "s", Role::leadVocal);
            auto f = evaluateVocal (t, false);
            juce::String st = "–";
            for (auto& x : f) if (x.id == "T06" || x.id == "T07") st = severityLabel (x.severity);
            md << "| +" << n1 (g, 0) << " dB | " << n1 (t.sibilantToVowelDb) << " dB | " << (g == 0 ? juce::String::fromUTF8 ("–") : "+" + n1 (t.sibilantToVowelDb - baseT.sibilantToVowelDb)) << " dB | "
               << t.sibilants.count << " | " << st << " |\n";
        }
        md << "\n";
    }

    const juce::File out (args.count ("out") ? args["out"] : juce::String ("nonius-kalibrierung.md"));
    out.replaceWithText (md, false, false, "\n");
    std::printf ("%s", md.toRawUTF8());
    return 0;
}
