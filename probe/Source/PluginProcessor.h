#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>

/*  Nonius Probe (Phase 0)
    Verändert den Klang nicht. Schreibt pro Instanz eine Log-Datei nach
    Dokumente/NoniusProbe/, um diese Fragen zu klären:
      1. Wann ist das Plugin-Fenster sichtbar? (Editor sichtbar/unsichtbar)
      2. Läuft processBlock auf stummen/deaktivierten Spuren weiter?
      3. Läuft processBlock bei gestopptem Transport weiter?
      4. Melden alle Instanzen dieselbe Zeitposition (inkl. Latenzausgleich)?
      5. Kommt der Spurname an?
      6. Laufen alle Instanzen im selben Prozess?
      7. Funktioniert Shared Memory des Betriebssystems zwischen Instanzen/Prozessen?
*/

class ProbeLog;
class SharedMemoryTest;

class NoniusProbeProcessor final : public juce::AudioProcessor,
                                   private juce::Timer
{
public:
    NoniusProbeProcessor();
    ~NoniusProbeProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nonius Probe"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    void updateTrackProperties (const TrackProperties& properties) override;

    // Für den Editor (Message-Thread)
    int getInstanceId() const noexcept { return instanceId; }
    juce::String getTrackName() const { return trackName; }
    juce::String getLogPath() const;
    juce::String getStatusText() const;
    void addMarker();
    void logEditorVisibility (bool visible);
    void logEvent (const juce::String& event, const juce::String& details = {});

private:
    void timerCallback() override;

    int instanceId = 0;
    juce::String trackName;
    int markerCount = 0;

    std::unique_ptr<ProbeLog> log;
    std::unique_ptr<SharedMemoryTest> shm;

    // Vom Audio-Thread geschrieben, vom Message-Thread gelesen
    std::atomic<juce::int64> blockCounter { 0 };
    std::atomic<juce::int64> lastTimeInSamples { -1 };
    std::atomic<juce::int64> lastBlockTicks { 0 };
    std::atomic<bool> lastIsPlaying { false };
    std::atomic<bool> lastHasPosition { false };
    std::atomic<bool> lastNonRealtime { false };
    std::atomic<float> peakSinceLastStatus { 0.0f };

    juce::int64 blocksAtLastStatus = 0;
    double lastStatusTime = 0.0;
    bool lastLoggedPlaying = false;
    bool lastLoggedNonRealtime = false;
    juce::String lastStatusLine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoniusProbeProcessor)
};
