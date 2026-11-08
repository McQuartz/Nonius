// NONIUS Mess-Kern: Grundbausteine (Filter, FFT, Bänder, Statistik).
// Reines C++17, keine JUCE-Abhängigkeit, damit Plugin und Test-Programm denselben Code nutzen.
#pragma once

#include <complex>
#include <cstddef>
#include <vector>

namespace nonius
{
    constexpr double kPi = 3.14159265358979323846;

    // ---------------------------------------------------------------- Biquad
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1 = 0, z2 = 0;

        double process (double x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        void reset() noexcept { z1 = z2 = 0; }

        // RBJ-Kochbuch-Formeln
        static Biquad highShelf (double sr, double f0, double gainDb, double q);
        static Biquad highPass  (double sr, double f0, double q);
        static Biquad lowPass   (double sr, double f0, double q);
        static Biquad bandPass  (double sr, double f0, double q);
    };

    // K-Filter nach ITU-R BS.1770 (für beliebige Samplerate neu berechnet).
    struct KWeighting
    {
        Biquad shelf, highpass;
        explicit KWeighting (double sr);
        double process (double x) noexcept { return highpass.process (shelf.process (x)); }
    };

    // A-Bewertung (IEC 61672), als Kaskade von Biquads.
    struct AWeighting
    {
        std::vector<Biquad> stages;
        double gain = 1.0;
        explicit AWeighting (double sr);
        double process (double x) noexcept;
    };

    // ---------------------------------------------------------------- FFT
    // In-place Radix-2-FFT, n muss eine Zweierpotenz sein.
    void fft (std::vector<std::complex<double>>& data);

    std::vector<double> hannWindow (std::size_t n);

    // Leistungsspektrum (|X|^2, einseitig, n/2+1 Werte) eines gefensterten Blocks.
    std::vector<double> powerSpectrum (const float* x, std::size_t n, const std::vector<double>& window);

    // ---------------------------------------------------------------- Bänder
    struct Band
    {
        double low, centre, high;
    };

    // Bruchteil-Oktav-Bänder (z. B. 6 = 1/6 Oktave), Mitten bei 1000 * 2^(k/fraction).
    std::vector<Band> fractionalOctaveBands (int fraction, double fMin, double fMax);

    // Bark-Bänder (kritische Bänder nach Zwicker), bis fMax.
    std::vector<Band> barkBands (double fMax);

    // Summiert ein Leistungsspektrum in Bänder. Zu schmale Bänder bekommen den nächsten Bin.
    std::vector<double> bandPowers (const std::vector<double>& spectrum, double sr, std::size_t fftSize,
                                    const std::vector<Band>& bands);

    // ---------------------------------------------------------------- Statistik
    double toDb (double power, double floorDb = -150.0);       // 10*log10
    double ampToDb (double amplitude, double floorDb = -150.0); // 20*log10
    double percentile (std::vector<double> values, double p);   // p in 0..100, lineare Interpolation
    double median (std::vector<double> values);
    double mean (const std::vector<double>& values);
}
