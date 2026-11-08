#include "LiveAnalyser.h"

#include "nonius/dsp.h"

#include <algorithm>
#include <cmath>

namespace nonius
{
    namespace
    {
        constexpr std::size_t kSpectrumSize = 8192;

        int severityRank (Severity s)
        {
            switch (s)
            {
                case Severity::problem: return 0;
                case Severity::warning: return 1;
                case Severity::info:    return 2;
                case Severity::noData:  return 3;
                case Severity::ok:      return 4;
            }
            return 5;
        }

        bool startsWith (const std::string& s, char c) { return ! s.empty() && s[0] == c; }
    }

    LiveAnalyser::LiveAnalyser (std::shared_ptr<const ReferenceProfile> p)
        : juce::Thread ("Nonius Analyse"), refProfile (std::move (p))
    {
        centres = refProfile->octaveCentres;
        if (centres.empty()) centres = { 31.5, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
        window = hannWindow (kSpectrumSize);
        scratch.resize (kSpectrumSize);
        publish (std::make_shared<Snapshot>());
    }

    LiveAnalyser::~LiveAnalyser() { stopThread (4000); }

    void LiveAnalyser::prepare (double sampleRate, int numChannels)
    {
        stopThread (4000);

        sr = sampleRate > 0 ? sampleRate : 48000.0;
        channelsIn = juce::jlimit (1, 2, numChannels);

        const int fifoSize = (int) (sr * 2.0); // 2 s Reserve, falls der Analyse-Thread kurz hängt
        fifo = std::make_unique<juce::AbstractFifo> (fifoSize);
        fifoBuffer.setSize (2, fifoSize);

        history.assign ((std::size_t) channelsIn, std::vector<float> ((std::size_t) (sr * historySeconds), 0.0f));
        histPos = histFilled = newSinceAnalysis = newSinceSpectrum = 0;
        bufferedSecs = 0.0;
        smoothedPower.assign (centres.size(), 0.0);
        {
            const juce::SpinLock::ScopedLockType l (liveLock);
            liveDb.clear();
        }
        publish (std::make_shared<Snapshot>());

        startThread (juce::Thread::Priority::low);
    }

    void LiveAnalyser::push (const float* const* ch, int numChannels, int n) noexcept
    {
        if (fifo == nullptr || numChannels <= 0 || n <= 0) return;
        const auto scope = fifo->write (n);  // was nicht passt, wird verworfen
        auto copy = [&] (int start, int size, int offset)
        {
            if (size <= 0) return;
            for (int c = 0; c < 2; ++c)
                fifoBuffer.copyFrom (c, start, ch[juce::jmin (c, numChannels - 1)] + offset, size);
        };
        copy (scope.startIndex1, scope.blockSize1, 0);
        copy (scope.startIndex2, scope.blockSize2, scope.blockSize1);
    }

    std::shared_ptr<const Snapshot> LiveAnalyser::latest() const
    {
        const juce::SpinLock::ScopedLockType l (snapLock);
        return snapshot;
    }

    void LiveAnalyser::publish (std::shared_ptr<const Snapshot> s)
    {
        const juce::SpinLock::ScopedLockType l (snapLock);
        snapshot = std::move (s);
    }

    std::vector<double> LiveAnalyser::liveOctaves() const
    {
        const juce::SpinLock::ScopedLockType l (liveLock);
        return liveDb;
    }

    // ------------------------------------------------------------------ Thread
    void LiveAnalyser::run()
    {
        while (! threadShouldExit())
        {
            wait (40);

            if (resetRequested.exchange (false))
            {
                histPos = histFilled = newSinceAnalysis = newSinceSpectrum = 0;
                bufferedSecs = 0.0;
                std::fill (smoothedPower.begin(), smoothedPower.end(), 0.0);
                {
                    const juce::SpinLock::ScopedLockType l (liveLock);
                    liveDb.clear();
                }
                auto empty = std::make_shared<Snapshot>();
                empty->role = getRole();
                publish (empty);
            }

            drainFifo();
            updateLiveSpectrum();

            const auto now = juce::Time::getMillisecondCounter();
            const bool enoughData = histFilled >= (std::size_t) (sr * 5.0);
            const bool due = now - lastAnalysisMs >= analysisIntervalMs && newSinceAnalysis >= (std::size_t) sr;
            if (enoughData && (due || roleChanged.exchange (false)))
                analyse();
        }
    }

    void LiveAnalyser::drainFifo()
    {
        if (fifo == nullptr) return;
        const int ready = fifo->getNumReady();
        if (ready <= 0) return;

        const auto scope = fifo->read (ready);
        const std::size_t cap = history[0].size();
        auto take = [&] (int start, int size)
        {
            for (int i = 0; i < size; ++i)
            {
                for (std::size_t c = 0; c < history.size(); ++c)
                    history[c][histPos] = fifoBuffer.getSample ((int) c, start + i);
                histPos = (histPos + 1) % cap;
            }
        };
        take (scope.startIndex1, scope.blockSize1);
        take (scope.startIndex2, scope.blockSize2);

        histFilled = std::min (cap, histFilled + (std::size_t) ready);
        newSinceAnalysis += (std::size_t) ready;
        newSinceSpectrum += (std::size_t) ready;
        bufferedSecs = (double) histFilled / sr;
    }

    void LiveAnalyser::updateLiveSpectrum()
    {
        if (newSinceSpectrum < (std::size_t) (sr * 0.04) || histFilled < kSpectrumSize) return;
        newSinceSpectrum = 0;

        const std::size_t cap = history[0].size();
        const std::size_t start = (histPos + cap - kSpectrumSize) % cap;
        for (std::size_t i = 0; i < kSpectrumSize; ++i)
        {
            const std::size_t k = (start + i) % cap;
            float v = 0;
            for (const auto& ch : history) v += ch[k];
            scratch[i] = v / (float) history.size();
        }

        const auto spec = powerSpectrum (scratch.data(), kSpectrumSize, window);
        const double binHz = sr / (double) kSpectrumSize;
        double total = 0;
        std::vector<double> bands (centres.size(), 0.0);
        for (std::size_t b = 1; b < spec.size(); ++b)
        {
            const double f = (double) b * binHz;
            if (f < 20.0 || f > 20000.0) continue;
            total += spec[b];
            for (std::size_t i = 0; i < centres.size(); ++i)
                if (f >= centres[i] / std::sqrt (2.0) && f < centres[i] * std::sqrt (2.0)) { bands[i] += spec[b]; break; }
        }
        if (total <= 1e-20) return; // Stille: Anzeige stehen lassen

        // Glättung ca. 0,4 s
        std::vector<double> out (centres.size());
        for (std::size_t i = 0; i < centres.size(); ++i)
        {
            const double rel = bands[i] / total;
            smoothedPower[i] = smoothedPower[i] <= 0 ? rel : smoothedPower[i] + 0.12 * (rel - smoothedPower[i]);
            out[i] = toDb (smoothedPower[i]);
        }
        const juce::SpinLock::ScopedLockType l (liveLock);
        liveDb = std::move (out);
    }

    void LiveAnalyser::analyse()
    {
        const auto t0 = juce::Time::getMillisecondCounter();
        newSinceAnalysis = 0;
        roleChanged = false;

        // Ringpuffer in zeitlicher Reihenfolge auslesen
        AudioBuffer audio;
        audio.sampleRate = sr;
        const std::size_t cap = history[0].size();
        const std::size_t start = (histPos + cap - histFilled) % cap;
        audio.channels.resize (history.size());
        for (std::size_t c = 0; c < history.size(); ++c)
        {
            auto& dst = audio.channels[c];
            dst.resize (histFilled);
            for (std::size_t i = 0; i < histFilled; ++i) dst[i] = history[c][(start + i) % cap];
        }

        auto s = std::make_shared<Snapshot>();
        s->role = getRole();
        s->valid = true;
        s->sampleRate = sr;
        s->secondsMeasured = (double) histFilled / sr;

        const Role coreRole = isVocal (s->role)                ? Role::leadVocal
                            : s->role == PluginRole::beat      ? Role::beat
                                                               : Role::sum;
        s->track = analyseTrack (audio, "", coreRole);
        if (threadShouldExit()) return;

        const auto& t = s->track;
        const auto& prof = *refProfile;
        s->octaveCentres = centres;
        s->octaveBalance = octaveBalance (t, centres);
        s->four = fourBandBalance (t);
        s->sideToMid = sideToMidDb (audio);
        s->spreadLu = t.loudness.shortTermP90 - t.loudness.shortTermP10;
        {
            const std::size_t frame = (std::size_t) (sr * 0.02);
            std::vector<double> lv;
            for (std::size_t i = 0; i + frame <= audio.numSamples(); i += frame)
            {
                double p = 0;
                for (const auto& ch : audio.channels)
                    for (std::size_t j = i; j < i + frame; ++j) p += (double) ch[j] * ch[j];
                lv.push_back (toDb (p / (double) (frame * audio.channels.size())));
            }
            if (lv.size() > 50)
            {
                auto sorted = lv;
                std::sort (sorted.begin(), sorted.end());
                const double p95 = sorted[sorted.size() * 95 / 100];
                std::vector<double> act; // digitale Stille / lange Pausen ausklammern
                for (double v : sorted) if (v > p95 - 60.0) act.push_back (v);
                if (act.size() > 50) s->gapDepthDb = p95 - act[act.size() / 10];
            }
        }

        auto& out = s->findings;
        if (s->role == PluginRole::rawVocal)
        {
            // Rohaufnahme: Aufnahme-Checks gelten, Dynamik nur als Info
            out = evaluateVocal (t, true);
        }
        else if (s->role == PluginRole::leadVocal)
        {
            // Bearbeitete Spur: Aufnahme-Checks passen hier nicht (Hall/Delay füllen die Pausen,
            // Pegel ist nach Kompressor/Limiter bewusst hoch), außer DC-Offset
            for (auto& f : evaluateVocal (t, false))
                if (! startsWith (f.id, 'A') || f.id == "A09") out.push_back (std::move (f));
        }
        else
        {
            const bool isSum = s->role == PluginRole::sum;
            for (auto& f : evaluateBeat (t))
            {
                if (isSum && f.id == "M12" && f.severity != Severity::ok)
                    f.fix = "Limiter am Ende der Summe, Ceiling −1 dBTP";
                if (isSum && f.id == "INFO") f.title = "Lautheit Summe";
                out.push_back (std::move (f));
            }
            for (auto& f : evaluateAgainstProfile (t, s->sideToMid, prof, isSum))
            {
                // Beat ohne Vocal: in den Mitten fehlt die Stimme naturgemäß, dort nicht gegen fertige Mixe bewerten
                if (! isSum && f.id == "M14" && f.frequency >= 250.0 && f.frequency <= 4000.0) continue;
                if (! isSum) f.note = "Beat allein gegen fertige Mixe, nur grob vergleichbar";
                out.push_back (std::move (f));
            }
        }

        std::stable_sort (out.begin(), out.end(), [] (const Finding& a, const Finding& b)
        {
            const int ra = severityRank (a.severity), rb = severityRank (b.severity);
            if (ra != rb) return ra < rb;
            return a.priority > b.priority;
        });

        publish (s);

        // Nächste Auswertung frühestens nach dem Dreifachen der Rechenzeit (CPU schonen bei vielen Instanzen)
        const auto took = juce::Time::getMillisecondCounter() - t0;
        lastAnalysisMs = juce::Time::getMillisecondCounter();
        analysisIntervalMs = juce::jmax ((juce::uint32) 2500, took * 3);
    }
}
