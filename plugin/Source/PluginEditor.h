// Oberfläche "N3": Kopfzeile mit Rolle und Stufen, Klangbild gegen Ziel, Befund, Werte gegen Ziel.
#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class NoniusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NoniusLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

class NoniusEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoniusEditor (NoniusProcessor&);
    ~NoniusEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    struct Meter
    {
        juce::String label, valueText;
        bool hasValue = false;
        double value = 0, lo = 0, hi = 0, mid = 0;
        bool hasMid = false, lowerIsFine = false, hasTarget = true; // lowerIsFine: unter dem Ziel ist kein Fehler
    };

    void timerCallback() override;
    void rebuildLists();
    std::vector<Meter> buildMeters() const;
    bool targetMode() const;

    void paintHeader (juce::Graphics&);
    void paintSpectrum (juce::Graphics&, juce::Rectangle<int> area);
    void paintFindings (juce::Graphics&, juce::Rectangle<int> area);
    void paintMeters (juce::Graphics&, juce::Rectangle<int> area);

    NoniusProcessor& proc;
    NoniusLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 500 };

    juce::ComboBox roleBox;
    juce::TextButton centralButton, holdButton { "Festhalten" }, measureButton { "Messen" };

    std::shared_ptr<const nonius::Snapshot> shown;
    std::vector<double> live;
    std::vector<const nonius::Finding*> issues, hints; // gefiltert nach Stufe
    bool frozen = false, showAll = false, wasPlaying = false;
    juce::String shownTrackName;

    juce::Rectangle<int> headerArea, spectrumArea, findingsArea, metersArea;
    juce::Rectangle<int> stageRects[5], moreRect, logoRect { 10, 6, 130, 36 };
    void showInfoMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoniusEditor)
};
