#pragma once

#include "PluginProcessor.h"

class NoniusProbeEditor final : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    explicit NoniusProbeEditor (NoniusProbeProcessor&);
    ~NoniusProbeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    NoniusProbeProcessor& processor;
    juce::TextButton markerButton { "Marker setzen" };
    juce::Label info;
    bool wasShowing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoniusProbeEditor)
};
