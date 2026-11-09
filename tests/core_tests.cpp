// Mess-Kern gegen synthetische Signale mit bekanntem Ergebnis prüfen.
#include "nonius/analysis.h"
#include "nonius/profile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

using namespace nonius;

static int failures = 0;

static void check (bool ok, const char* what, double value, double expected)
{
    std::printf ("%s %-55s %9.3f (erwartet %9.3f)\n", ok ? "OK  " : "FAIL", what, value, expected);
    if (! ok) ++failures;
}

static void near (const char* what, double value, double expected, double tol)
{
    check (std::fabs (value - expected) <= tol, what, value, expected);
}

static AudioBuffer sine (double sr, double f, double amp, double seconds, int chs = 2, double phase = 0.0)
{
    AudioBuffer a;
    a.sampleRate = sr;
    const auto n = (std::size_t) (sr * seconds);
    a.channels.assign ((std::size_t) chs, std::vector<float> (n));
    for (std::size_t i = 0; i < n; ++i)
    {
        const auto v = (float) (amp * std::sin (2.0 * kPi * f * (double) i / sr + phase));
        for (auto& c : a.channels) c[i] = v;
    }
    return a;
}

static std::vector<float> noise (std::size_t n, double amp, unsigned seed)
{
    std::mt19937 rng (seed);
    std::normal_distribution<double> d (0.0, amp);
    std::vector<float> v (n);
    for (auto& x : v) x = (float) d (rng);
    return v;
}

int main()
{
    // --- Lautheit: 997-Hz-Sinus, 0 dBFS auf L und R = −3,01 LUFS (EBU Tech 3341)
    for (double sr : { 44100.0, 48000.0 })
    {
        auto m = sine (sr, 997.0, 1.0, 20.0, 1);
        near ("BS.1770: 997 Hz 0 dBFS ein Kanal → −3,01 LUFS", measureLoudness (m).integratedLufs, -3.01, 0.1);
        auto a = sine (sr, 997.0, 1.0, 20.0);
        near ("BS.1770: 997 Hz 0 dBFS stereo → 0 LUFS (Tech 3341)", measureLoudness (a).integratedLufs, 0.0, 0.1);
        auto b = sine (sr, 997.0, std::pow (10.0, -20.0 / 20.0), 20.0);
        near ("BS.1770: 997 Hz −20 dBFS stereo → −20 LUFS", measureLoudness (b).integratedLufs, -20.0, 0.1);
    }

    // --- EBU Tech 3341 Fall 1 sinngemäß: Gating. 20 s bei −20 dBFS, dann 20 s bei −60 dBFS (fällt raus)
    {
        auto a = sine (48000, 1000, std::pow (10.0, -20.0 / 20.0), 20.0);
        auto b = sine (48000, 1000, std::pow (10.0, -60.0 / 20.0), 20.0);
        for (std::size_t c = 0; c < 2; ++c) a.channels[c].insert (a.channels[c].end(), b.channels[c].begin(), b.channels[c].end());
        near ("Gating: leiser Teil wird ignoriert", measureLoudness (a).integratedLufs, -20.0, 0.15);
    }

    // --- True Peak: Sinus fs/4 mit 45° Phase hat Samples bei 0,707, echte Spitze 1,0
    {
        auto a = sine (48000, 12000, 1.0, 1.0, 1, kPi / 4.0);
        double sp = 0; for (float v : a.channels[0]) sp = std::max (sp, (double) std::fabs (v));
        near ("Sample Peak fs/4, 45° → −3,01 dBFS", ampToDb (sp), -3.01, 0.05);
        near ("True Peak fs/4, 45° → 0 dBTP", measureTruePeakDb (a), 0.0, 0.3);
    }

    // --- Vocal gegen Beat: zwei Rauschsignale mit 3 dB Unterschied
    {
        const double sr = 44100; const std::size_t n = (std::size_t) (sr * 30);
        AudioBuffer v, b; v.sampleRate = b.sampleRate = sr;
        v.channels = { noise (n, 0.1 * std::pow (10.0, 3.0 / 20.0), 1) };
        b.channels = { noise (n, 0.1, 2) };
        auto m = analyseMix (v, b);
        near ("Vocal/Beat-Verhältnis Rauschen +3 dB", m.vocalToBeatDb, 3.0, 0.3);
    }

    // --- Zerfall mit bekanntem RT60: Rausch-Bursts mit exponentiellem Ausklang (RT 0,5 s)
    {
        const double sr = 44100, rt = 0.5;
        AudioBuffer a; a.sampleRate = sr;
        std::vector<float> x;
        auto nz = noise ((std::size_t) (sr * 60), 0.3, 3);
        std::size_t k = 0;
        for (int burst = 0; burst < 30; ++burst)
        {
            for (std::size_t i = 0; i < (std::size_t) (sr * 0.4); ++i) x.push_back (nz[k++]);                // Wort
            for (std::size_t i = 0; i < (std::size_t) (sr * 1.2); ++i)                                       // Ausklang
                x.push_back ((float) (nz[k++] * std::pow (10.0, -3.0 * ((double) i / sr) / rt)) + 1e-4f * nz[(k * 7) % nz.size()]);
        }
        a.channels = { x };
        auto t = analyseTrack (a, "rt", Role::leadVocal);
        near ("Blinde Nachhall-Schätzung RT 0,5 s", t.reverbSeconds, 0.5, 0.1);
    }

    // --- Resonanz: Rauschen + stehender Sinus bei 1 kHz wird gefunden
    {
        const double sr = 44100; const std::size_t n = (std::size_t) (sr * 20);
        AudioBuffer a; a.sampleRate = sr;
        auto x = noise (n, 0.05, 4);
        for (std::size_t i = 0; i < n; ++i) x[i] += (float) (0.05 * std::sin (2.0 * kPi * 1000.0 * (double) i / sr));
        a.channels = { x };
        auto t = analyseTrack (a, "res", Role::leadVocal);
        const double f = t.resonances.empty() ? 0.0 : t.resonances[0].frequency;
        near ("Resonanz bei 1 kHz gefunden", f, 1000.0, 40.0);
    }

    // --- Clipping: Sinus auf 0,5 hart begrenzt
    {
        auto a = sine (44100, 220, 1.0, 5.0, 1);
        for (auto& v : a.channels[0]) v = std::clamp (v, -0.5f, 0.5f);
        auto t = analyseTrack (a, "clip", Role::leadVocal);
        check (t.clippingRuns > 50, "Clipping erkannt (Stellen, 50 ms zusammengefasst)", t.clippingRuns, 100);
        auto b = sine (44100, 220, 0.5, 5.0, 1);
        auto t2 = analyseTrack (b, "noclip", Role::leadVocal);
        check (t2.clippingRuns == 0, "Kein Clipping bei sauberem Sinus", t2.clippingRuns, 0);
    }

    // --- Stereo: gegenphasig → Korrelation −1, Mono-Verlust groß
    {
        auto a = sine (44100, 440, 0.3, 5.0, 2);
        for (auto& v : a.channels[1]) v = -v;
        auto t = analyseTrack (a, "phase", Role::beat);
        near ("Korrelation gegenphasig", t.correlation, -1.0, 0.01);
        check (t.monoLossMaxDb > 20.0, "Mono-Verlust gegenphasig > 20 dB", t.monoLossMaxDb, 20.0);
    }


    // --- Vocal-Dynamik: "Silben" mit abwechselnd 0 / −8 dB, Pausen dazwischen
    {
        const double sr = 44100;
        AudioBuffer a; a.sampleRate = sr;
        std::vector<float> x ((std::size_t) (sr * 20), 0.0f);
        for (int k = 0; k < 80; ++k)
        {
            const double amp = (k % 2 == 0) ? 0.3 : 0.3 * std::pow (10.0, -8.0 / 20.0);
            const std::size_t s0 = (std::size_t) (k * 0.25 * sr), len = (std::size_t) (0.12 * sr);
            for (std::size_t i = 0; i < len && s0 + i < x.size(); ++i)
                x[s0 + i] = (float) (amp * std::sin (2.0 * kPi * 300.0 * (double) i / sr));
        }
        for (std::size_t i = 0; i < x.size(); ++i) x[i] += (float) (0.003 * std::sin (2.0 * kPi * 5000.0 * (double) i / sr)); // "Raum" −40 dB
        a.channels = { x };
        const auto d = measureVocalDynamics (a);
        near ("Silben-Dynamik 8 dB", d.syllableDb, 8.0, 0.6);
        near ("Lücke = Abstand Silbe zu Raum (ca. 40 dB)", d.gapDepthDb, 40.0, 3.0);

        // Profil: Ziel 4 dB Silben-Dynamik -> Rohspur bekommt K01 mit Gain Reduction
        ReferenceProfile p; p.songs = 10;
        p.ranges["Silben-Dynamik"] = { 3.0, 4.0, 5.5, true };
        auto t = analyseTrack (a, "v", Role::leadVocal);
        const auto f = evaluateVocalAgainstProfile (t, d, -150.0, p, true);
        bool k01 = false;
        for (const auto& x2 : f) if (x2.id == "K01" && x2.severity == Severity::warning && x2.fix.find ("Gain Reduction") != std::string::npos) k01 = true;
        check (k01, "Rohspur: K01 schlägt Kompression vor", k01 ? 1 : 0, 1);
        const auto g = evaluateVocalAgainstProfile (t, d, -150.0, p, false);
        bool k01w = false;
        for (const auto& x2 : g) if (x2.id == "K01" && x2.severity != Severity::ok) k01w = true;
        check (k01w, "Bearbeitet: 8 dB > P90 -> zu wenig Kompression", k01w ? 1 : 0, 1);
    }

    std::printf ("\n%s (%d Fehler)\n", failures == 0 ? "Alle Tests bestanden" : "TESTS FEHLGESCHLAGEN", failures);
    return failures == 0 ? 0 : 1;
}
