// Live-Messung: der Audio-Thread kopiert nur Samples in eine lock-freie FIFO,
// alles Rechnen passiert in einem eigenen Hintergrund-Thread mit dem Mess-Kern.
#pragma once

#include "nonius/analysis.h"
#include "nonius/profile.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <memory>
#include <vector>

namespace nonius
{
    // leadVocal = bearbeitete Vocal-Spur, rawVocal = Rohaufnahme (Zahlen bleiben fest, stehen im Projekt)
    enum class PluginRole { leadVocal = 0, beat = 1, sum = 2, rawVocal = 3 };
    inline bool isVocal (PluginRole r) { return r == PluginRole::leadVocal || r == PluginRole::rawVocal; }

    struct Snapshot
    {
        bool valid = false;
        PluginRole role = PluginRole::leadVocal;
        double secondsMeasured = 0;      // Länge des ausgewerteten Fensters
        double sampleRate = 0;
        TrackAnalysis track;
        std::vector<Finding> findings;    // sortiert: Probleme zuerst, dann nach Priorität
        std::vector<double> octaveCentres;
        std::vector<double> octaveBalance; // dB relativ zur Gesamtleistung
        FourBands four;
        double sideToMid = -150;
        double spreadLu = 0;              // Short-term P90 − P10
        VocalDynamics vocalDyn;           // Silben-Dynamik, Abstand Wort zu Lücke (nonius/profile.h)
    };

    class LiveAnalyser : private juce::Thread
    {
    public:
        static constexpr double historySeconds = 30.0;

        LiveAnalyser (std::shared_ptr<const ReferenceProfile> profile, std::shared_ptr<const ReferenceProfile> vocalProfile);
        ~LiveAnalyser() override;

        // Nicht gleichzeitig mit push() aufrufen (prepareToPlay).
        void prepare (double sampleRate, int numChannels);

        // Audio-Thread: nur kopieren, nie blockieren.
        void push (const float* const* channels, int numChannels, int numSamples) noexcept;

        void requestReset() noexcept           { resetRequested = true; }
        void setRole (PluginRole r) noexcept    { role = (int) r; roleChanged = true; }
        PluginRole getRole() const noexcept     { return (PluginRole) role.load(); }

        std::shared_ptr<const Snapshot> latest() const;

        // Live-Klangbalance pro Oktave (geglättet), gleiche Normierung wie das Profil.
        std::vector<double> liveOctaves() const;

        // Sekunden im Verlauf (für die Anzeige, bevor die erste Auswertung da ist)
        double bufferedSeconds() const noexcept { return bufferedSecs.load(); }

        const ReferenceProfile& profile() const noexcept { return *refProfile; }
        const ReferenceProfile& vocalProfile() const noexcept { return *vocProfile; }
        // Profil, gegen das eine Rolle bewertet wird (Vocals: Referenz-Vocals, sonst komplette Mixe)
        const ReferenceProfile& profileFor (PluginRole r) const noexcept
        {
            return isVocal (r) && ! vocProfile->empty() ? *vocProfile : *refProfile;
        }

    private:
        void run() override;
        void drainFifo();
        void updateLiveSpectrum();
        void analyse();
        void publish (std::shared_ptr<const Snapshot> s);

        std::shared_ptr<const ReferenceProfile> refProfile, vocProfile;
        std::vector<double> centres;

        // FIFO Audio-Thread -> Analyse-Thread
        std::unique_ptr<juce::AbstractFifo> fifo;
        juce::AudioBuffer<float> fifoBuffer;

        // Verlauf (Ringpuffer, nur im Analyse-Thread)
        std::vector<std::vector<float>> history;
        std::size_t histPos = 0, histFilled = 0, newSinceAnalysis = 0, newSinceSpectrum = 0;

        double sr = 48000.0;
        int channelsIn = 2;
        std::atomic<int> role { 0 };
        std::atomic<bool> resetRequested { false }, roleChanged { false };
        std::atomic<double> bufferedSecs { 0.0 };
        juce::uint32 lastAnalysisMs = 0, analysisIntervalMs = 2500;

        // Live-Spektrum
        std::vector<double> window, smoothedPower;
        std::vector<float> scratch;
        mutable juce::SpinLock liveLock;
        std::vector<double> liveDb;

        mutable juce::SpinLock snapLock;
        std::shared_ptr<const Snapshot> snapshot;
    };
}
