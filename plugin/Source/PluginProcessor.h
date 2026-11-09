#pragma once

#include "LiveAnalyser.h"

#include <juce_audio_processors/juce_audio_processors.h>

class NoniusProcessor : public juce::AudioProcessor
{
public:
    NoniusProcessor();
    ~NoniusProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nonius"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void updateTrackProperties (const TrackProperties& properties) override;

    // ---- für die Oberfläche
    nonius::LiveAnalyser& analyser() noexcept { return live; }
    bool isPlaying() const noexcept { return playing.load(); }

    nonius::PluginRole getRole() const noexcept { return live.getRole(); }
    void setRole (nonius::PluginRole r, bool chosenByUser);
    int getStage() const noexcept { return stage.load(); }
    void setStage (int s) noexcept { stage = juce::jlimit (1, 5, s); }

    juce::String getTrackName() const;
    juce::Colour getTrackColour() const;

private:
    static nonius::PluginRole guessRole (const juce::String& trackName, nonius::PluginRole fallback);

    nonius::LiveAnalyser live;
    std::atomic<bool> playing { false };
    std::atomic<int> stage { 5 };
    std::atomic<bool> roleChosen { false };

    mutable juce::CriticalSection trackLock;
    juce::String trackName;
    juce::Colour trackColour { juce::Colours::transparentBlack };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoniusProcessor)
};
