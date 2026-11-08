// Vergleich zweier Spuren: Vorher/Nachher und Vocal gegen Beat.
#include "nonius/analysis.h"

#include <algorithm>
#include <cmath>

namespace nonius
{
    // ------------------------------------------------------------------ Vorher/Nachher
    ProcessingDiff compareProcessing (const TrackAnalysis& before, const TrackAnalysis& after)
    {
        ProcessingDiff d;
        d.loudnessChangeLu = after.loudness.integratedLufs - before.loudness.integratedLufs;
        d.lraChangeLu = after.loudness.lra - before.loudness.lra;
        d.crest100msChangeDb = after.crest100msDb - before.crest100msDb;
        d.crest1sChangeDb = after.crest1sDb - before.crest1sDb;
        if (before.sibilantToVowelDb > -149 && after.sibilantToVowelDb > -149)
            d.sibilantChangeDb = after.sibilantToVowelDb - before.sibilantToVowelDb;
        if (before.noiseFloorReliable && after.noiseFloorReliable && before.noiseFloorDbA > -149.0 && after.noiseFloorDbA > -149.0)
            d.noiseChangeDb = (after.noiseFloorDbA - after.loudness.integratedLufs) - (before.noiseFloorDbA - before.loudness.integratedLufs);

        // Klangänderung: beide Spektren sind auf 200 Hz–5 kHz normiert, also schon "bei gleicher Lautheit".
        // Zusammengefasst auf Oktaven.
        const auto& bb = before.spectrum.bands;
        if (! bb.empty() && before.spectrum.levelDb.size() == bb.size() && after.spectrum.levelDb.size() == after.spectrum.bands.size())
        {
            d.octaveBands = fractionalOctaveBands (1, 31.5, std::min (bb.back().high, after.spectrum.bands.back().high));
            for (const auto& ob : d.octaveBands)
            {
                double pb = 0, pa = 0;
                for (std::size_t i = 0; i < bb.size(); ++i)
                    if (bb[i].centre >= ob.low && bb[i].centre < ob.high) pb += std::pow (10.0, before.spectrum.levelDb[i] / 10.0);
                const auto& ab = after.spectrum.bands;
                for (std::size_t i = 0; i < ab.size(); ++i)
                    if (ab[i].centre >= ob.low && ab[i].centre < ob.high) pa += std::pow (10.0, after.spectrum.levelDb[i] / 10.0);
                d.octaveChangeDb.push_back (toDb (pa) - toDb (pb));
            }
        }

        // Grobe Schätzung der Kompression: wie stark die Streuung der Short-term-Lautheit abnimmt.
        const double spreadBefore = before.loudness.shortTermP90 - before.loudness.shortTermP10;
        const double spreadAfter = after.loudness.shortTermP90 - after.loudness.shortTermP10;
        d.estimatedGainReductionDb = std::max (0.0, spreadBefore - spreadAfter);
        return d;
    }

    // ------------------------------------------------------------------ Vocal gegen Beat
    namespace
    {
        std::vector<float> aWeighted (const std::vector<float>& x, double sr)
        {
            AWeighting a (sr);
            std::vector<float> y (x.size());
            for (std::size_t i = 0; i < x.size(); ++i) y[i] = (float) a.process (x[i]);
            return y;
        }

        // Leistung in Fenstern fester Dauer (Sekunden), unabhängig von der Samplerate.
        std::vector<double> windowPowers (const std::vector<float>& x, double sr, double seconds)
        {
            const auto len = (std::size_t) std::lround (sr * seconds);
            std::vector<double> p;
            for (std::size_t s = 0; s + len <= x.size(); s += len)
            {
                double sum = 0.0;
                for (std::size_t i = s; i < s + len; ++i) sum += (double) x[i] * x[i];
                p.push_back (sum / (double) len);
            }
            return p;
        }

        // Bark-Bandleistungen auf einem gemeinsamen Zeitraster (Schritt in Sekunden).
        std::vector<std::vector<double>> barkFrames (const std::vector<float>& x, double sr, double hopSec,
                                                     const std::vector<Band>& bands, std::vector<double>& frameLevelDb)
        {
            int N = 1;
            while (N < sr * 0.046) N <<= 1; // 2048 bei 44,1/48 kHz
            const auto window = hannWindow ((std::size_t) N);
            std::vector<std::vector<double>> out;
            for (std::size_t f = 0;; ++f)
            {
                const auto start = (std::size_t) std::lround ((double) f * hopSec * sr);
                if (start + (std::size_t) N > x.size()) break;
                auto spec = powerSpectrum (&x[start], (std::size_t) N, window);
                double total = 0.0;
                for (double v : spec) total += v;
                frameLevelDb.push_back (toDb (total));
                out.push_back (bandPowers (spec, sr, (std::size_t) N, bands));
            }
            return out;
        }

        // Spreizfunktion nach Schroeder (dB), dz = Bark(maskiert) - Bark(Maskierer)
        double spreading (double dz)
        {
            const double t = dz + 0.474;
            return 15.81 + 7.5 * t - 17.5 * std::sqrt (1.0 + t * t);
        }
    }

    MixRelation analyseMix (const AudioBuffer& vocal, const AudioBuffer& beat)
    {
        MixRelation m;
        const auto vm = vocal.mono();
        const auto bm = beat.mono();
        m.overlapSeconds = std::min (vocal.durationSeconds(), beat.durationSeconds());

        // --- M01/M02: A-bewertetes Verhältnis in 1-s-Fenstern (nach Gerdes & Siedenburg 2023)
        {
            auto vp = windowPowers (aWeighted (vm, vocal.sampleRate), vocal.sampleRate, 1.0);
            auto bp = windowPowers (aWeighted (bm, beat.sampleRate), beat.sampleRate, 1.0);
            const std::size_t w = std::min (vp.size(), bp.size());
            std::vector<double> vDb;
            for (std::size_t i = 0; i < w; ++i) vDb.push_back (toDb (vp[i]));
            const double vLoud = percentile (vDb, 90);
            std::vector<double> ratios;
            for (std::size_t i = 0; i < w; ++i)
            {
                // nur Fenster, in denen das Vocal wirklich läuft und der Beat nicht schweigt
                if (vDb[i] > vLoud - 10.0 && toDb (bp[i]) > -70.0)
                    ratios.push_back (vDb[i] - toDb (bp[i]));
            }
            m.windowsUsed = (int) ratios.size();
            if (! ratios.empty())
            {
                m.vocalToBeatDb = median (ratios);
                m.vocalToBeatP10 = percentile (ratios, 10);
                m.vocalToBeatP90 = percentile (ratios, 90);
            }
        }

        // --- M03: Verdeckung über die Mithörschwelle des Beats (Bark-Bänder, vereinfachtes MPEG-Modell)
        {
            const double hop = 1024.0 / 44100.0;
            const double fMax = std::min (vocal.sampleRate, beat.sampleRate) * 0.45;
            m.barkBandsUsed = barkBands (fMax);
            const auto& bands = m.barkBandsUsed;
            std::vector<double> vLvl, bLvl;
            auto vf = barkFrames (vm, vocal.sampleRate, hop, bands, vLvl);
            auto bf = barkFrames (bm, beat.sampleRate, hop, bands, bLvl);
            const std::size_t frames = std::min (vf.size(), bf.size());

            std::vector<double> vAll (vLvl.begin(), vLvl.begin() + (long) std::min (frames, vLvl.size()));
            const double vActive = percentile (vAll, 90) - 20.0;

            // pro Band: typischer Pegel des Vocals, um nur Bänder zu werten, in denen das Vocal Energie hat
            std::vector<double> bandMedian (bands.size());
            for (std::size_t b = 0; b < bands.size(); ++b)
            {
                std::vector<double> v;
                for (std::size_t f = 0; f < frames; ++f)
                    if (vLvl[f] > vActive) v.push_back (toDb (vf[f][b]));
                bandMedian[b] = median (v);
            }

            constexpr double offsetDb = 6.0; // Abstand Mithörschwelle unter der gespreizten Beat-Energie (geräuschartiger Maskierer)
            std::vector<int> masked (bands.size(), 0), counted (bands.size(), 0);
            std::vector<std::vector<double>> deficit (bands.size());
            for (std::size_t f = 0; f < frames; ++f)
            {
                if (vLvl[f] <= vActive || bLvl[f] < -90.0) continue;
                for (std::size_t i = 0; i < bands.size(); ++i)
                {
                    const double vDb = toDb (vf[f][i]);
                    if (vDb < bandMedian[i] - 10.0) continue; // Vocal hier gerade ohne Energie
                    double thr = 0.0;
                    for (std::size_t j = 0; j < bands.size(); ++j)
                        thr += bf[f][j] * std::pow (10.0, spreading ((double) i - (double) j) / 10.0);
                    const double thrDb = toDb (thr) - offsetDb;
                    ++counted[i];
                    if (vDb < thrDb) { ++masked[i]; deficit[i].push_back (thrDb - vDb); }
                }
            }

            m.maskedFractionPerBark.resize (bands.size(), 0.0);
            for (std::size_t i = 0; i < bands.size(); ++i)
                m.maskedFractionPerBark[i] = counted[i] > 20 ? (double) masked[i] / counted[i] : 0.0;

            // Zusammenhängende Bereiche > 30 %, nur 150 Hz–8 kHz (dort trägt die Stimme Verständlichkeit)
            std::size_t i = 0;
            while (i < bands.size())
            {
                if (m.maskedFractionPerBark[i] > 0.3 && bands[i].low >= 200.0 && bands[i].low < 8000.0)
                {
                    MaskedRegion r;
                    r.lowHz = bands[i].low;
                    double fr = 0; int c = 0; std::vector<double> def;
                    while (i < bands.size() && m.maskedFractionPerBark[i] > 0.3)
                    {
                        r.highHz = bands[i].high;
                        fr = std::max (fr, m.maskedFractionPerBark[i]); ++c;
                        def.insert (def.end(), deficit[i].begin(), deficit[i].end());
                        ++i;
                    }
                    r.maskedFraction = fr;
                    r.neededDb = median (def);
                    m.maskedRegions.push_back (r);
                }
                else ++i;
            }
        }

        // --- Summe Vocal + Beat (nur bei gleicher Samplerate)
        if (vocal.sampleRate == beat.sampleRate)
        {
            AudioBuffer sum;
            sum.sampleRate = vocal.sampleRate;
            const std::size_t len = std::min (vocal.numSamples(), beat.numSamples());
            const std::size_t chs = std::max (vocal.channels.size(), beat.channels.size());
            sum.channels.assign (chs, std::vector<float> (len, 0.0f));
            for (std::size_t c = 0; c < chs; ++c)
            {
                const auto& vc = vocal.channels[std::min (c, vocal.channels.size() - 1)];
                const auto& bc = beat.channels[std::min (c, beat.channels.size() - 1)];
                for (std::size_t i = 0; i < len; ++i) sum.channels[c][i] = vc[i] + bc[i];
            }
            m.sumTruePeakDb = measureTruePeakDb (sum);
            m.sumIntegratedLufs = measureLoudness (sum).integratedLufs;
        }

        return m;
    }
}
