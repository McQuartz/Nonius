#include "nonius/dsp.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace nonius
{
    // ---------------------------------------------------------------- Biquad
    static Biquad normalise (double b0, double b1, double b2, double a0, double a1, double a2)
    {
        Biquad q;
        q.b0 = b0 / a0; q.b1 = b1 / a0; q.b2 = b2 / a0;
        q.a1 = a1 / a0; q.a2 = a2 / a0;
        return q;
    }

    Biquad Biquad::highShelf (double sr, double f0, double gainDb, double q)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * f0 / sr;
        const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
        const double sA = 2.0 * std::sqrt (A) * alpha;
        return normalise (A * ((A + 1) + (A - 1) * cw + sA),
                          -2 * A * ((A - 1) + (A + 1) * cw),
                          A * ((A + 1) + (A - 1) * cw - sA),
                          (A + 1) - (A - 1) * cw + sA,
                          2 * ((A - 1) - (A + 1) * cw),
                          (A + 1) - (A - 1) * cw - sA);
    }

    Biquad Biquad::highPass (double sr, double f0, double q)
    {
        const double w0 = 2.0 * kPi * f0 / sr;
        const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
        return normalise ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + alpha, -2 * cw, 1 - alpha);
    }

    Biquad Biquad::lowPass (double sr, double f0, double q)
    {
        const double w0 = 2.0 * kPi * f0 / sr;
        const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
        return normalise ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + alpha, -2 * cw, 1 - alpha);
    }

    Biquad Biquad::bandPass (double sr, double f0, double q)
    {
        const double w0 = 2.0 * kPi * f0 / sr;
        const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
        return normalise (alpha, 0, -alpha, 1 + alpha, -2 * cw, 1 - alpha);
    }

    // ---------------------------------------------------------------- K-Filter
    // Parameter aus BS.1770 / libebur128 (bilineare Transformation für beliebige Samplerate).
    KWeighting::KWeighting (double sr)
    {
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (kPi * f0 / sr);
            const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            shelf.b0 = (Vh + Vb * K / Q + K * K) / a0;
            shelf.b1 = 2.0 * (K * K - Vh) / a0;
            shelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
            shelf.a1 = 2.0 * (K * K - 1.0) / a0;
            shelf.a2 = (1.0 - K / Q + K * K) / a0;
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (kPi * f0 / sr);
            const double a0 = 1.0 + K / Q + K * K;
            highpass.b0 = 1.0; highpass.b1 = -2.0; highpass.b2 = 1.0;
            highpass.a1 = 2.0 * (K * K - 1.0) / a0;
            highpass.a2 = (1.0 - K / Q + K * K) / a0;
        }
    }

    // ---------------------------------------------------------------- A-Bewertung
    // Pole bei 20.6 Hz (2x), 107.7 Hz, 737.9 Hz, 12194 Hz (2x); Nullstellen 4x bei 0 Hz.
    // Umsetzung: 2 Hochpässe 2. Ordnung (20.6 Hz), je 1 Hochpass 1. Ordnung als Biquad (107.7, 737.9),
    // 1 Tiefpass 2. Ordnung (12194 Hz, doppelter Pol). Normierung auf 0 dB bei 1 kHz.
    static Biquad firstOrderHighPass (double sr, double fc)
    {
        const double K = std::tan (kPi * fc / sr);
        Biquad q;
        q.b0 = 1.0 / (1.0 + K); q.b1 = -q.b0; q.b2 = 0;
        q.a1 = (K - 1.0) / (K + 1.0); q.a2 = 0;
        return q;
    }

    static Biquad doublePoleHighPass (double sr, double fc)
    {
        // (s/(s+w))^2, bilinear mit Vorverzerrung
        const double K = std::tan (kPi * fc / sr);
        const double a0 = (1 + K) * (1 + K);
        Biquad q;
        q.b0 = 1.0 / a0; q.b1 = -2.0 / a0; q.b2 = 1.0 / a0;
        q.a1 = 2.0 * (K * K - 1.0) / a0;
        q.a2 = (1 - K) * (1 - K) / a0;
        return q;
    }

    static Biquad doublePoleLowPass (double sr, double fc)
    {
        const double K = std::tan (kPi * std::min (fc, sr * 0.45) / sr);
        const double a0 = (1 + K) * (1 + K);
        Biquad q;
        q.b0 = K * K / a0; q.b1 = 2 * K * K / a0; q.b2 = K * K / a0;
        q.a1 = 2.0 * (K * K - 1.0) / a0;
        q.a2 = (1 - K) * (1 - K) / a0;
        return q;
    }

    static double magnitudeAt (const Biquad& q, double f, double sr)
    {
        const std::complex<double> z = std::polar (1.0, -2.0 * kPi * f / sr);
        const auto num = q.b0 + q.b1 * z + q.b2 * z * z;
        const auto den = 1.0 + q.a1 * z + q.a2 * z * z;
        return std::abs (num / den);
    }

    AWeighting::AWeighting (double sr)
    {
        stages.push_back (doublePoleHighPass (sr, 20.598997));
        stages.push_back (firstOrderHighPass (sr, 107.65265));
        stages.push_back (firstOrderHighPass (sr, 737.86223));
        stages.push_back (doublePoleLowPass (sr, 12194.217));

        double m = 1.0;
        for (auto& s : stages)
            m *= magnitudeAt (s, 1000.0, sr);
        gain = 1.0 / m;
    }

    double AWeighting::process (double x) noexcept
    {
        double y = x * gain;
        for (auto& s : stages)
            y = s.process (y);
        return y;
    }

    // ---------------------------------------------------------------- FFT
    void fft (std::vector<std::complex<double>>& a)
    {
        const std::size_t n = a.size();
        for (std::size_t i = 1, j = 0; i < n; ++i)
        {
            std::size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap (a[i], a[j]);
        }

        for (std::size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = -2.0 * kPi / (double) len;
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (std::size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1.0, 0.0);
                for (std::size_t k = 0; k < len / 2; ++k)
                {
                    const auto u = a[i + k];
                    const auto v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
    }

    std::vector<double> hannWindow (std::size_t n)
    {
        std::vector<double> w (n);
        for (std::size_t i = 0; i < n; ++i)
            w[i] = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) n);
        return w;
    }

    std::vector<double> powerSpectrum (const float* x, std::size_t n, const std::vector<double>& window)
    {
        std::vector<std::complex<double>> buf (n);
        double wsum = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            buf[i] = { (double) x[i] * window[i], 0.0 };
            wsum += window[i] * window[i];
        }

        fft (buf);

        // Normierung: Summe aller Bins = mittlere Leistung des Blocks (Parseval, einseitig).
        std::vector<double> p (n / 2 + 1);
        const double norm = 1.0 / (wsum * (double) n);
        for (std::size_t k = 0; k <= n / 2; ++k)
        {
            double v = std::norm (buf[k]) * norm;
            if (k != 0 && k != n / 2)
                v *= 2.0;
            p[k] = v;
        }
        return p;
    }

    // ---------------------------------------------------------------- Bänder
    std::vector<Band> fractionalOctaveBands (int fraction, double fMin, double fMax)
    {
        std::vector<Band> bands;
        const double half = std::pow (2.0, 0.5 / fraction);
        const int kMin = (int) std::ceil (fraction * std::log2 (fMin / 1000.0));
        const int kMax = (int) std::floor (fraction * std::log2 (fMax / 1000.0));
        for (int k = kMin; k <= kMax; ++k)
        {
            const double c = 1000.0 * std::pow (2.0, (double) k / fraction);
            bands.push_back ({ c / half, c, c * half });
        }
        return bands;
    }

    std::vector<Band> barkBands (double fMax)
    {
        static const double edges[] = { 20, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720,
                                        2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };
        std::vector<Band> bands;
        for (std::size_t i = 0; i + 1 < sizeof (edges) / sizeof (edges[0]); ++i)
        {
            if (edges[i + 1] > fMax)
                break;
            bands.push_back ({ edges[i], std::sqrt (edges[i] * edges[i + 1]), edges[i + 1] });
        }
        return bands;
    }

    std::vector<double> bandPowers (const std::vector<double>& spectrum, double sr, std::size_t fftSize,
                                    const std::vector<Band>& bands)
    {
        const double binHz = sr / (double) fftSize;
        std::vector<double> out (bands.size(), 0.0);
        for (std::size_t b = 0; b < bands.size(); ++b)
        {
            const auto lo = (std::size_t) std::ceil (bands[b].low / binHz);
            const auto hi = (std::size_t) std::floor (bands[b].high / binHz);
            double sum = 0.0;
            if (hi >= lo)
            {
                for (std::size_t k = lo; k <= hi && k < spectrum.size(); ++k)
                    sum += spectrum[k];
            }
            else
            {
                // Band schmaler als ein Bin: anteilige Leistung des nächsten Bins.
                const auto k = std::min (spectrum.size() - 1, (std::size_t) std::lround (bands[b].centre / binHz));
                sum = spectrum[k] * (bands[b].high - bands[b].low) / binHz;
            }
            out[b] = sum;
        }
        return out;
    }

    // ---------------------------------------------------------------- Statistik
    double toDb (double power, double floorDb)
    {
        return power > 0.0 ? std::max (floorDb, 10.0 * std::log10 (power)) : floorDb;
    }

    double ampToDb (double amplitude, double floorDb)
    {
        return amplitude > 0.0 ? std::max (floorDb, 20.0 * std::log10 (amplitude)) : floorDb;
    }

    double percentile (std::vector<double> v, double p)
    {
        if (v.empty())
            return 0.0;
        std::sort (v.begin(), v.end());
        const double pos = std::clamp (p, 0.0, 100.0) / 100.0 * (double) (v.size() - 1);
        const auto i = (std::size_t) std::floor (pos);
        const double frac = pos - (double) i;
        return i + 1 < v.size() ? v[i] * (1.0 - frac) + v[i + 1] * frac : v[i];
    }

    double median (std::vector<double> v) { return percentile (std::move (v), 50.0); }

    double mean (const std::vector<double>& v)
    {
        return v.empty() ? 0.0 : std::accumulate (v.begin(), v.end(), 0.0) / (double) v.size();
    }
}
