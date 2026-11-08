// Messungen einer einzelnen Spur.
#include "nonius/analysis.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace nonius
{
    namespace
    {
        struct FrameLevels
        {
            std::size_t frameLen = 0;
            std::vector<double> rmsDb;  // pro Frame
            std::vector<double> peakDb;
        };

        FrameLevels frameLevels (const std::vector<float>& x, std::size_t frameLen)
        {
            FrameLevels f;
            f.frameLen = frameLen;
            const std::size_t frames = x.size() / frameLen;
            f.rmsDb.resize (frames);
            f.peakDb.resize (frames);
            for (std::size_t i = 0; i < frames; ++i)
            {
                double s = 0.0, pk = 0.0;
                for (std::size_t j = i * frameLen; j < (i + 1) * frameLen; ++j)
                {
                    s += (double) x[j] * x[j];
                    pk = std::max (pk, (double) std::fabs (x[j]));
                }
                f.rmsDb[i] = toDb (s / (double) frameLen);
                f.peakDb[i] = ampToDb (pk);
            }
            return f;
        }

        // Aktivitäts-Schwelle: 30 dB unter den lauten Teilen, aber mindestens -65 dBFS.
        double activityThreshold (const std::vector<double>& rmsDb)
        {
            std::vector<double> nonSilent;
            for (double v : rmsDb)
                if (v > -120.0) nonSilent.push_back (v);
            if (nonSilent.empty())
                return 0.0;
            return std::max (-65.0, percentile (nonSilent, 95) - 30.0);
        }

        double bandSum (const std::vector<double>& powers, const std::vector<Band>& bands, double lo, double hi)
        {
            double s = 0.0;
            for (std::size_t i = 0; i < bands.size(); ++i)
                if (bands[i].centre >= lo && bands[i].centre < hi)
                    s += powers[i];
            return s;
        }

        int nextPow2 (double v)
        {
            int n = 1;
            while (n < v) n <<= 1;
            return n;
        }
    }

    // ==================================================================
    TrackAnalysis analyseTrack (const AudioBuffer& audio, const std::string& name, Role role)
    {
        TrackAnalysis t;
        t.name = name;
        t.role = role;
        t.sampleRate = audio.sampleRate;
        t.durationSeconds = audio.durationSeconds();
        t.lossySource = audio.lossySource;
        t.channels = (int) audio.channels.size();

        const double sr = audio.sampleRate;
        const auto mono = audio.mono();
        const std::size_t n = mono.size();
        if (n < (std::size_t) (sr * 2))
            return t;

        // ---------------------------------------------------------- Peaks, DC, Clipping
        double peak = 0.0;
        for (const auto& ch : audio.channels)
        {
            double sum = 0.0, chPeak = 0.0;
            for (float v : ch)
            {
                sum += v;
                chPeak = std::max (chPeak, (double) std::fabs (v));
            }
            peak = std::max (peak, chPeak);
            t.dcOffsetPercent = std::max (t.dcOffsetPercent, std::fabs (sum / (double) ch.size()) * 100.0);

            // Flache Spitzen: >= 3 Samples in Folge auf (fast) demselben Wert nahe dem Kanal-Maximum.
            if (chPeak > 0.25)
            {
                const double limit = chPeak * 0.9995;
                std::size_t run = 0;
                std::size_t lastRunEnd = 0;
                for (std::size_t i = 0; i < ch.size(); ++i)
                {
                    if (std::fabs (ch[i]) >= limit)
                        ++run;
                    else
                    {
                        if (run >= 3 && (t.clippingRuns == 0 || i - lastRunEnd > (std::size_t) (sr * 0.05)))
                        {
                            ++t.clippingRuns;
                            lastRunEnd = i;
                        }
                        run = 0;
                    }
                }
            }
        }
        t.samplePeakDb = ampToDb (peak);
        t.truePeakDb = measureTruePeakDb (audio);

        // ---------------------------------------------------------- 100-ms-Frames, Aktivität
        const auto frame100 = (std::size_t) std::lround (sr * 0.1);
        // Pegel aus der mittleren Leistung der Kanäle (nicht aus Mono), damit gegenphasige
        // Anteile nicht als Stille zählen.
        std::vector<float> power (n, 0.0f);
        for (const auto& ch : audio.channels)
            for (std::size_t i = 0; i < n; ++i)
                power[i] += ch[i] * ch[i] / (float) audio.channels.size();
        for (auto& v : power) v = std::sqrt (v);
        auto f100 = frameLevels (power, frame100);
        const double actThr = activityThreshold (f100.rmsDb);
        std::vector<bool> active100 (f100.rmsDb.size());
        int activeCount = 0, hot = 0;
        double activePeak = 0.0;
        // Peak pro Frame über alle Kanäle (nicht nur Mono)
        std::vector<double> framePeakAll (f100.rmsDb.size(), 0.0);
        for (const auto& ch : audio.channels)
            for (std::size_t i = 0; i < framePeakAll.size(); ++i)
                for (std::size_t j = i * frame100; j < (i + 1) * frame100; ++j)
                    framePeakAll[i] = std::max (framePeakAll[i], (double) std::fabs (ch[j]));

        for (std::size_t i = 0; i < f100.rmsDb.size(); ++i)
        {
            active100[i] = f100.rmsDb[i] > actThr;
            if (active100[i])
            {
                ++activeCount;
                activePeak = std::max (activePeak, framePeakAll[i]);
                if (ampToDb (framePeakAll[i]) > -3.0) ++hot;
            }
        }
        t.activeSeconds = activeCount * 0.1;
        t.activePeakDb = ampToDb (activePeak);
        t.hotFramePercent = activeCount > 0 ? 100.0 * hot / activeCount : 0.0;

        // ---------------------------------------------------------- Rauschen in Pausen (A-bewertet)
        {
            AWeighting aw (sr);
            std::vector<float> a (n);
            for (std::size_t i = 0; i < n; ++i)
                a[i] = (float) aw.process (mono[i]);
            auto fa = frameLevels (a, frame100);
            std::vector<double> pauses;
            int digitalSilence = 0;
            for (std::size_t i = 0; i < fa.rmsDb.size() && i < active100.size(); ++i)
            {
                // Pause: deutlich unter der Aktivitätsschwelle, Nachbarn ebenfalls inaktiv (kein Ausklingen)
                const bool neighboursQuiet = (i == 0 || ! active100[i - 1]) && (i + 1 >= active100.size() || ! active100[i + 1]);
                if (! active100[i] && neighboursQuiet)
                {
                    if (fa.rmsDb[i] < -110.0) ++digitalSilence;
                    else pauses.push_back (fa.rmsDb[i]);
                }
            }
            if (pauses.size() >= 20)
            {
                t.noiseFloorDbA = percentile (pauses, 20);
                t.noiseFloorReliable = true;
            }
            else if (digitalSilence >= 20)
            {
                t.noiseFloorDbA = -150.0; // Pausen digital still (geschnitten/Gate)
                t.noiseFloorReliable = true;
            }
        }

        // ---------------------------------------------------------- Lautheit, Crest, Wortspitzen
        t.loudness = measureLoudness (audio);
        {
            std::vector<double> c100;
            for (std::size_t i = 0; i < f100.rmsDb.size(); ++i)
                if (active100[i]) c100.push_back (f100.peakDb[i] - f100.rmsDb[i]);
            t.crest100msDb = median (c100);

            std::vector<double> c1s;
            for (std::size_t s = 0; s + 10 <= f100.rmsDb.size(); s += 10)
            {
                int act = 0; double pw = 0.0, pk = -150.0;
                for (std::size_t i = s; i < s + 10; ++i)
                {
                    if (active100[i]) ++act;
                    pw += std::pow (10.0, f100.rmsDb[i] / 10.0);
                    pk = std::max (pk, f100.peakDb[i]);
                }
                if (act >= 5)
                    c1s.push_back (pk - toDb (pw / 10.0));
            }
            t.crest1sDb = median (c1s);
        }

        {
            // K03: 20-ms-RMS mehr als 10 dB über dem aktiven Mittel der umgebenden 3 s
            const auto frame20 = (std::size_t) std::lround (sr * 0.02);
            auto f20 = frameLevels (mono, frame20);
            const int half = 75; // 1,5 s je Seite
            std::vector<double> pw (f20.rmsDb.size());
            for (std::size_t i = 0; i < pw.size(); ++i) pw[i] = std::pow (10.0, f20.rmsDb[i] / 10.0);
            double lastEvent = -1.0;
            for (std::size_t i = 0; i < f20.rmsDb.size(); ++i)
            {
                if (f20.rmsDb[i] <= actThr) continue;
                double s = 0.0; int c = 0;
                const std::size_t a = i > (std::size_t) half ? i - half : 0;
                const std::size_t b = std::min (f20.rmsDb.size(), i + half);
                for (std::size_t j = a; j < b; ++j)
                    if (f20.rmsDb[j] > actThr) { s += pw[j]; ++c; }
                if (c < 20) continue;
                if (f20.rmsDb[i] > toDb (s / c) + 10.0)
                {
                    const double time = (double) i * 0.02;
                    if (time - lastEvent > 0.3)
                        t.wordPeaks.timesSeconds.push_back (time);
                    lastEvent = time;
                }
            }
            t.wordPeaks.count = (int) t.wordPeaks.timesSeconds.size();
        }

        // ---------------------------------------------------------- Popps (A07)
        {
            Biquad lp1 = Biquad::lowPass (sr, 100.0, 0.7071), lp2 = Biquad::lowPass (sr, 100.0, 0.7071);
            const auto frame5 = (std::size_t) std::lround (sr * 0.005);
            std::vector<float> low (n);
            for (std::size_t i = 0; i < n; ++i)
                low[i] = (float) lp2.process (lp1.process (mono[i]));
            auto fl = frameLevels (low, frame5);
            auto fb = frameLevels (mono, frame5);
            double lastEvent = -1.0;
            for (std::size_t i = 100; i < fl.rmsDb.size(); ++i)
            {
                // lokales Bass-Mittel der letzten 500 ms
                double s = 0.0;
                for (std::size_t j = i - 100; j < i; ++j) s += std::pow (10.0, fl.rmsDb[j] / 10.0);
                const double localMean = toDb (s / 100.0);
                const bool jump = fl.rmsDb[i] - fl.rmsDb[i - 2] >= 12.0;          // Anstieg < 10 ms
                const bool aboveLocal = fl.rmsDb[i] >= localMean + 10.0;
                const bool dominant = fl.rmsDb[i] >= fb.rmsDb[i] - 3.0;          // Bass (< 100 Hz) dominiert den Frame
                const bool loudEnough = fb.rmsDb[i] > actThr;
                if (jump && aboveLocal && dominant && loudEnough)
                {
                    const double time = (double) i * 0.005;
                    if (time - lastEvent > 0.15)
                        t.plosives.timesSeconds.push_back (time);
                    lastEvent = time;
                }
            }
            t.plosives.count = (int) t.plosives.timesSeconds.size();
        }

        const double activeMinutes = std::max (t.activeSeconds / 60.0, 1.0 / 60.0);
        t.wordPeaks.perMinute = t.wordPeaks.count / activeMinutes;
        t.plosives.perMinute = t.plosives.count / activeMinutes;

        // ---------------------------------------------------------- Spektrum (1/6 Okt.), Resonanzen, Brummen
        {
            const int N = nextPow2 (sr * 0.17); // 8192 bei 44,1/48 kHz
            const int hop = N / 2;
            const auto window = hannWindow ((std::size_t) N);
            const double fMax = std::min (16000.0, sr * 0.45);
            auto bands = fractionalOctaveBands (6, 31.5, fMax);
            auto fine = fractionalOctaveBands (24, 150.0, std::min (8000.0, fMax));
            t.spectrum.bands = bands;

            std::vector<double> sumPow (bands.size(), 0.0);
            std::vector<std::vector<double>> frameDb (bands.size());
            std::vector<double> pauseSpectrum;
            int pauseFrames = 0;

            std::vector<int> fineHits (fine.size(), 0);
            std::vector<double> fineExcessSum (fine.size(), 0.0);
            int frames = 0;

            for (std::size_t start = 0; start + (std::size_t) N <= n; start += (std::size_t) hop)
            {
                double s = 0.0;
                for (std::size_t i = start; i < start + (std::size_t) N; ++i) s += (double) mono[i] * mono[i];
                const double lvl = toDb (s / N);
                auto spec = powerSpectrum (&mono[start], (std::size_t) N, window);

                if (lvl <= actThr)
                {
                    if (lvl > -110.0 && lvl < actThr - 10.0)
                    {
                        if (pauseSpectrum.empty()) pauseSpectrum.assign (spec.size(), 0.0);
                        for (std::size_t k = 0; k < spec.size(); ++k) pauseSpectrum[k] += spec[k];
                        ++pauseFrames;
                    }
                    continue;
                }

                ++frames;
                auto bp = bandPowers (spec, sr, (std::size_t) N, bands);
                const double ref = bandSum (bp, bands, 200.0, 5000.0);
                for (std::size_t b = 0; b < bands.size(); ++b)
                {
                    sumPow[b] += bp[b];
                    frameDb[b].push_back (toDb (bp[b] / std::max (ref, 1e-20)));
                }

                // Resonanzen: 1/24 Okt. minus geglättet (±1/6 Okt. Mittel in dB-Leistung)
                auto fp = bandPowers (spec, sr, (std::size_t) N, fine);
                for (std::size_t b = 4; b + 4 < fine.size(); ++b)
                {
                    double sm = 0.0;
                    for (std::size_t j = b - 4; j <= b + 4; ++j) if (j != b) sm += fp[j];
                    sm /= 8.0;
                    const double ex = toDb (fp[b]) - toDb (sm);
                    if (ex >= 6.0) { ++fineHits[b]; fineExcessSum[b] += ex; }
                }
            }

            t.spectrum.activeFrames = frames;
            if (frames > 0)
            {
                const double ref = bandSum (sumPow, bands, 200.0, 5000.0);
                t.spectrum.levelDb.resize (bands.size());
                t.spectrum.p10Db.resize (bands.size());
                t.spectrum.p90Db.resize (bands.size());
                for (std::size_t b = 0; b < bands.size(); ++b)
                {
                    t.spectrum.levelDb[b] = toDb (sumPow[b] / ref);
                    t.spectrum.p10Db[b] = percentile (frameDb[b], 10);
                    t.spectrum.p90Db[b] = percentile (frameDb[b], 90);
                }

                // Neigung: lineare Regression dB über log2(f), 100 Hz–10 kHz
                double sx = 0, sy = 0, sxx = 0, sxy = 0; int c = 0;
                for (std::size_t b = 0; b < bands.size(); ++b)
                    if (bands[b].centre >= 100.0 && bands[b].centre <= 10000.0)
                    {
                        const double x = std::log2 (bands[b].centre), y = t.spectrum.levelDb[b];
                        sx += x; sy += y; sxx += x * x; sxy += x * y; ++c;
                    }
                if (c > 2)
                    t.spectrum.tiltDbPerOct = (c * sxy - sx * sy) / (c * sxx - sx * sx);

                t.rumbleDb = toDb (bandSum (sumPow, bands, 0.0, 80.0) / ref);
                t.proximityDb = toDb (bandSum (sumPow, bands, 100.0, 250.0) / ref);
                t.airDb = fMax > 10000.0 ? toDb (bandSum (sumPow, bands, 10000.0, 1e9) / ref) : -150.0;

                // Resonanz-Treffer: lokale Maxima der Beständigkeit
                for (std::size_t b = 1; b + 1 < fine.size(); ++b)
                {
                    const double pers = (double) fineHits[b] / frames;
                    if (pers >= 0.7 && fineHits[b] >= fineHits[b - 1] && fineHits[b] >= fineHits[b + 1])
                        t.resonances.push_back ({ fine[b].centre, fineExcessSum[b] / fineHits[b], pers });
                }
                std::sort (t.resonances.begin(), t.resonances.end(),
                           [] (const ResonanceHit& a, const ResonanceHit& b) { return a.persistence * a.heightDb > b.persistence * b.heightDb; });
                if (t.resonances.size() > 3) t.resonances.resize (3);
            }

            // Brummen: 50/60-Hz-Reihe im Pausen-Spektrum
            if (pauseFrames >= 10)
            {
                const double binHz = sr / N;
                auto excessAt = [&] (double f)
                {
                    const auto k0 = (std::size_t) std::lround (f / binHz);
                    double pk = 0.0;
                    for (std::size_t k = k0 - 1; k <= k0 + 1; ++k) pk = std::max (pk, pauseSpectrum[k]);
                    std::vector<double> nb;
                    for (std::size_t k = (std::size_t) std::lround ((f - 40) / binHz); k <= (std::size_t) std::lround ((f + 40) / binHz); ++k)
                        if (k + 3 < k0 || k > k0 + 3) nb.push_back (pauseSpectrum[k]);
                    if (nb.size() < 4) return 0.0;
                    return toDb (pk) - toDb (median (nb));
                };
                for (double base : { 50.0, 60.0 })
                {
                    const double ex = (excessAt (base) + excessAt (2 * base) + excessAt (3 * base)) / 3.0;
                    if (ex > t.humExcessDb) { t.humExcessDb = ex; t.humFrequency = base; }
                }
            }
        }

        // ---------------------------------------------------------- Zischlaute (T06/T07)
        {
            const int N = nextPow2 (sr * 0.0116); // 512 bei 44,1/48 kHz
            const auto window = hannWindow ((std::size_t) N);
            const double binHz = sr / N;
            const auto kHi = (std::size_t) std::ceil (4500.0 / binHz);
            std::vector<double> sLevels, vowelLevels;
            double sFreqWeighted = 0.0, sFreqWeight = 0.0;
            int run = 0; double runPeak = -150.0;
            const double frameSec = (double) (N / 2) / sr;

            auto closeRun = [&] (double time)
            {
                const double dur = run * frameSec;
                if (run > 0 && dur >= 0.02 && dur <= 0.25)
                {
                    t.sibilants.timesSeconds.push_back (time - dur);
                    sLevels.push_back (runPeak);
                }
                run = 0; runPeak = -150.0;
            };

            std::size_t idx = 0;
            for (std::size_t start = 0; start + (std::size_t) N <= n; start += (std::size_t) N / 2, ++idx)
            {
                auto spec = powerSpectrum (&mono[start], (std::size_t) N, window);
                double total = 0.0, hi = 0.0, centroidNum = 0.0, hiNum = 0.0;
                for (std::size_t k = 1; k < spec.size(); ++k)
                {
                    total += spec[k];
                    centroidNum += spec[k] * k * binHz;
                    if (k >= kHi) { hi += spec[k]; hiNum += spec[k] * k * binHz; }
                }
                const double lvl = toDb (total);
                const double centroid = total > 0 ? centroidNum / total : 0.0;
                const double time = (double) start / sr;

                const bool loud = lvl > actThr - 10.0;
                const bool isS = loud && total > 0 && hi / total > 0.5 && centroid > 4500.0;
                if (isS)
                {
                    ++run;
                    runPeak = std::max (runPeak, lvl);
                    sFreqWeighted += hiNum; sFreqWeight += hi;
                }
                else
                {
                    closeRun (time);
                    if (lvl > actThr && centroid < 2000.0)
                        vowelLevels.push_back (lvl);
                }
            }
            closeRun ((double) n / sr);

            t.sibilants.count = (int) t.sibilants.timesSeconds.size();
            t.sibilants.perMinute = t.sibilants.count / activeMinutes;
            if (sLevels.size() >= 5 && vowelLevels.size() >= 50)
                t.sibilantToVowelDb = median (sLevels) - percentile (vowelLevels, 75);
            t.sibilantFrequency = sFreqWeight > 0 ? sFreqWeighted / sFreqWeight : 0.0;
        }

        // ---------------------------------------------------------- Nachhall (blind, experimentell)
        {
            // Hüllkurve 10 ms im Sprachband (300 Hz–4 kHz)
            Biquad hp = Biquad::highPass (sr, 300.0, 0.7071), lp = Biquad::lowPass (sr, 4000.0, 0.7071);
            std::vector<float> bp (n);
            for (std::size_t i = 0; i < n; ++i) bp[i] = (float) lp.process (hp.process (mono[i]));
            const auto frame10 = (std::size_t) std::lround (sr * 0.01);
            auto fe = frameLevels (bp, frame10);
            const auto& e = fe.rmsDb;

            std::vector<double> rts;
            std::size_t i = 1;
            while (i + 2 < e.size())
            {
                // Start: lokales Maximum über der Aktivitätsschwelle
                if (! (e[i] > actThr && e[i] >= e[i - 1] && e[i] >= e[i + 1])) { ++i; continue; }
                const double top = e[i];
                std::size_t j = i + 1;
                double minSoFar = top;
                bool gateCut = false;
                while (j < e.size())
                {
                    if (e[j] < -110.0) { gateCut = true; break; }
                    if (e[j] > minSoFar + 1.5) break;  // wieder lauter: neues Wort
                    minSoFar = std::min (minSoFar, e[j]);
                    if (top - e[j] >= 25.0) break;
                    ++j;
                }
                if (! gateCut && top - minSoFar >= 20.0)
                {
                    // Regression im Bereich -5 bis -20 dB unter dem Maximum
                    double sx = 0, sy = 0, sxx = 0, sxy = 0; int c = 0;
                    for (std::size_t k = i; k <= j && k < e.size(); ++k)
                    {
                        const double d = top - e[k];
                        if (d >= 5.0 && d <= 20.0)
                        {
                            const double x = (double) (k - i) * 0.01, y = e[k];
                            sx += x; sy += y; sxx += x * x; sxy += x * y; ++c;
                        }
                    }
                    if (c >= 4)
                    {
                        const double slope = (c * sxy - sx * sy) / (c * sxx - sx * sx); // dB/s
                        if (slope < 0.0)
                        {
                            const double rt = -60.0 / slope;
                            if (rt > 0.08 && rt < 3.0) rts.push_back (rt);
                        }
                    }
                }
                i = std::max (i + 1, j);
            }

            t.reverbDecays = (int) rts.size();
            if (rts.size() >= 5)
            {
                // Freie Ausklingvorgänge sind die steilsten, die der Raum zulässt: unteres Perzentil.
                t.reverbSeconds = percentile (rts, 20);
                const double spread = percentile (rts, 40) / std::max (1e-3, percentile (rts, 10));
                t.reverbConfidence = std::clamp (std::min (1.0, rts.size() / 30.0) * (spread < 1.6 ? 1.0 : 0.7), 0.0, 1.0);
            }
        }

        // ---------------------------------------------------------- Stereo
        if (audio.channels.size() >= 2)
        {
            const auto& L = audio.channels[0];
            const auto& R = audio.channels[1];
            double ll = 0, rr = 0, lr = 0;
            for (std::size_t f = 0; f < active100.size(); ++f)
            {
                if (! active100[f]) continue;
                for (std::size_t k = f * frame100; k < (f + 1) * frame100; ++k)
                {
                    ll += (double) L[k] * L[k]; rr += (double) R[k] * R[k]; lr += (double) L[k] * R[k];
                }
            }
            t.correlation = (ll > 0 && rr > 0) ? lr / std::sqrt (ll * rr) : 1.0;
            t.balanceDb = (ll > 0 && rr > 0) ? 10.0 * std::log10 (ll / rr) : 0.0;

            Biquad m1 = Biquad::lowPass (sr, 120.0, 0.7071), m2 = m1, s1 = m1, s2 = m1;
            double mm = 0, ss = 0;
            for (std::size_t k = 0; k < n; ++k)
            {
                const double mv = m2.process (m1.process (0.5 * (L[k] + R[k])));
                const double sv = s2.process (s1.process (0.5 * (L[k] - R[k])));
                mm += mv * mv; ss += sv * sv;
            }
            t.sideBelow120Db = (mm > 0) ? toDb (ss / mm) : -150.0;

            // Mono-Verlust pro Oktavband: Leistung (L+R)/2 gegen mittlere Leistung von L und R
            const int N = nextPow2 (sr * 0.09);
            const auto window = hannWindow ((std::size_t) N);
            auto oct = fractionalOctaveBands (1, 31.5, std::min (16000.0, sr * 0.45));
            std::vector<double> pm (oct.size(), 0.0), plr (oct.size(), 0.0);
            std::vector<float> mid ((std::size_t) N);
            for (std::size_t start = 0; start + (std::size_t) N <= n; start += (std::size_t) N)
            {
                const std::size_t fIdx = start / frame100;
                if (fIdx < active100.size() && ! active100[fIdx]) continue;
                for (int k = 0; k < N; ++k) mid[(std::size_t) k] = 0.5f * (L[start + (std::size_t) k] + R[start + (std::size_t) k]);
                auto bm = bandPowers (powerSpectrum (mid.data(), (std::size_t) N, window), sr, (std::size_t) N, oct);
                auto bl = bandPowers (powerSpectrum (&L[start], (std::size_t) N, window), sr, (std::size_t) N, oct);
                auto br = bandPowers (powerSpectrum (&R[start], (std::size_t) N, window), sr, (std::size_t) N, oct);
                for (std::size_t b = 0; b < oct.size(); ++b) { pm[b] += bm[b]; plr[b] += 0.5 * (bl[b] + br[b]); }
            }
            for (std::size_t b = 0; b < oct.size(); ++b)
            {
                if (plr[b] <= 0) continue;
                const double loss = std::min (60.0, -10.0 * std::log10 (std::max (pm[b], 1e-30) / plr[b]));
                if (loss > t.monoLossMaxDb) { t.monoLossMaxDb = loss; t.monoLossFrequency = oct[b].centre; }
            }
        }

        return t;
    }
}
