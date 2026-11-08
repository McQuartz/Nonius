// Lautheit nach ITU-R BS.1770-4 / EBU R128 und True Peak (4x Oversampling).
#include "nonius/analysis.h"

#include <algorithm>
#include <cmath>

namespace nonius
{
    std::vector<float> AudioBuffer::mono() const
    {
        std::vector<float> m (numSamples(), 0.0f);
        if (channels.empty())
            return m;
        const float g = 1.0f / (float) channels.size();
        for (const auto& ch : channels)
            for (std::size_t i = 0; i < m.size(); ++i)
                m[i] += ch[i] * g;
        return m;
    }

    static double powerToLufs (double p) { return p > 0 ? -0.691 + 10.0 * std::log10 (p) : -150.0; }

    LoudnessResult measureLoudness (const AudioBuffer& audio)
    {
        LoudnessResult r;
        const std::size_t n = audio.numSamples();
        const auto block = (std::size_t) std::lround (audio.sampleRate * 0.1);
        if (n < block * 4 || block == 0)
            return r;

        // Mittlere Leistung pro 100-ms-Block, K-gefiltert, Kanäle summiert (Gewicht 1 für L/R).
        const std::size_t numBlocks = n / block;
        std::vector<double> blockPower (numBlocks, 0.0);
        for (const auto& ch : audio.channels)
        {
            KWeighting k (audio.sampleRate);
            for (std::size_t b = 0; b < numBlocks; ++b)
            {
                double s = 0.0;
                for (std::size_t i = b * block; i < (b + 1) * block; ++i)
                {
                    const double y = k.process (ch[i]);
                    s += y * y;
                }
                blockPower[b] += s / (double) block;
            }
        }
        // Mono-Dateien werden wie BS.1770 behandelt (ein Kanal). Für Vocal-Spuren, die als
        // Stereo mit identischen Kanälen exportiert sind, ergibt das +3 dB gegenüber Mono, wie in jedem Meter.

        auto windowPower = [&] (std::size_t start, std::size_t len)
        {
            double s = 0.0;
            for (std::size_t i = start; i < start + len; ++i)
                s += blockPower[i];
            return s / (double) len;
        };

        // Momentary: 400 ms, Schritt 100 ms
        std::vector<double> momentaryPower;
        for (std::size_t b = 0; b + 4 <= numBlocks; ++b)
        {
            const double p = windowPower (b, 4);
            momentaryPower.push_back (p);
            r.momentary.push_back (powerToLufs (p));
        }

        // Integrated mit absolutem (-70 LUFS) und relativem (-10 LU) Gate
        {
            double sum = 0.0; int cnt = 0;
            for (double p : momentaryPower)
                if (powerToLufs (p) > -70.0) { sum += p; ++cnt; }
            if (cnt > 0)
            {
                const double rel = powerToLufs (sum / cnt) - 10.0;
                double s2 = 0.0; int c2 = 0;
                for (double p : momentaryPower)
                {
                    const double l = powerToLufs (p);
                    if (l > -70.0 && l > rel) { s2 += p; ++c2; }
                }
                r.integratedLufs = c2 > 0 ? powerToLufs (s2 / c2) : -150.0;
            }
        }

        // Short-term: 3 s. Für LRA Schritt 100 ms, für die Zeitreihe Schritt 1 s.
        std::vector<double> stFine;
        for (std::size_t b = 0; b + 30 <= numBlocks; ++b)
        {
            const double l = powerToLufs (windowPower (b, 30));
            stFine.push_back (l);
            if (b % 10 == 0)
                r.shortTerm.push_back (l);
        }

        {
            std::vector<double> gated;
            double sum = 0.0;
            for (double l : stFine)
                if (l > -70.0) { gated.push_back (l); sum += std::pow (10.0, (l + 0.691) / 10.0); }
            if (! gated.empty())
            {
                const double rel = powerToLufs (sum / (double) gated.size()) - 20.0;
                std::vector<double> g2;
                for (double l : gated)
                    if (l > rel) g2.push_back (l);
                if (! g2.empty())
                    r.lra = percentile (g2, 95) - percentile (g2, 10);
            }

            std::vector<double> active;
            for (double l : r.shortTerm)
                if (l > -70.0) active.push_back (l);
            if (! active.empty())
            {
                // Streuung nur über Fenster mit Signal (leise Teile < Integrated - 20 LU verwerfen)
                std::vector<double> loud;
                for (double l : active)
                    if (l > r.integratedLufs - 20.0) loud.push_back (l);
                if (loud.empty()) loud = active;
                r.shortTermP10 = percentile (loud, 10);
                r.shortTermP90 = percentile (loud, 90);
            }
        }

        for (double l : r.momentary)
            r.maxMomentary = std::max (r.maxMomentary, l);

        return r;
    }

    // ------------------------------------------------------------------ True Peak
    // 4x Oversampling mit Kaiser-gefenstertem Sinc (48 Taps pro Phase).
    static double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 50; ++k)
        {
            term *= (x / (2.0 * k)) * (x / (2.0 * k));
            sum += term;
            if (term < 1e-12 * sum) break;
        }
        return sum;
    }

    double measureTruePeakDb (const AudioBuffer& audio)
    {
        constexpr int factor = 4, tapsPerPhase = 48, taps = factor * tapsPerPhase;
        constexpr double beta = 8.0;

        // Prototyp-Tiefpass bei 0.5/factor * 0.95 (normierte Frequenz bezogen auf die hohe Rate)
        std::vector<double> h (taps);
        const double fc = 0.5 / factor * 0.95;
        const double centre = (taps - 1) / 2.0;
        for (int i = 0; i < taps; ++i)
        {
            const double t = i - centre;
            const double sinc = t == 0 ? 2.0 * fc : std::sin (2.0 * kPi * fc * t) / (kPi * t);
            const double r = (2.0 * i) / (taps - 1) - 1.0;
            const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / besselI0 (beta);
            h[i] = sinc * w * factor;
        }

        // Polyphasen
        std::vector<std::vector<double>> phases (factor, std::vector<double> (tapsPerPhase));
        for (int p = 0; p < factor; ++p)
            for (int k = 0; k < tapsPerPhase; ++k)
                phases[p][k] = h[k * factor + p];

        double peak = 0.0;
        for (const auto& ch : audio.channels)
        {
            const std::size_t n = ch.size();
            for (std::size_t i = 0; i < n; ++i)
            {
                peak = std::max (peak, (double) std::fabs (ch[i]));
                // Zwischenwerte nur in der Nähe lauter Samples berechnen (spart Zeit, ändert das Ergebnis nicht).
                if (std::fabs (ch[i]) < 0.5 * peak && peak > 0.0)
                    continue;
                for (int p = 0; p < factor; ++p)
                {
                    double acc = 0.0;
                    for (int k = 0; k < tapsPerPhase; ++k)
                    {
                        const long idx = (long) i - k + tapsPerPhase / 2;
                        if (idx >= 0 && idx < (long) n)
                            acc += phases[p][k] * ch[(std::size_t) idx];
                    }
                    peak = std::max (peak, std::fabs (acc));
                }
            }
        }
        return ampToDb (peak);
    }
}
