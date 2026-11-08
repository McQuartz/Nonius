#include "PluginEditor.h"

NoniusProbeEditor::NoniusProbeEditor (NoniusProbeProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    markerButton.onClick = [this] { processor.addMarker(); };
    addAndMakeVisible (markerButton);

    info.setJustificationType (juce::Justification::topLeft);
    info.setColour (juce::Label::textColourId, juce::Colour (0xffecebe7));
    addAndMakeVisible (info);

    processor.logEvent ("FENSTER_ERSTELLT");
    setSize (520, 240);
    startTimerHz (5);
}

NoniusProbeEditor::~NoniusProbeEditor()
{
    processor.logEvent ("FENSTER_GESCHLOSSEN");
}

void NoniusProbeEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff151617));
    g.setColour (juce::Colour (0xfff2a541));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("NONIUS PROBE  #" + juce::String (processor.getInstanceId()), 16, 12, 400, 24, juce::Justification::left);
}

void NoniusProbeEditor::resized()
{
    markerButton.setBounds (getWidth() - 156, 12, 140, 28);
    info.setBounds (16, 48, getWidth() - 32, getHeight() - 60);
}

void NoniusProbeEditor::timerCallback()
{
    // Frage 1: Sichtbarkeit des Fensters mitschreiben, wenn sie sich ändert.
    // isShowing() allein hat im ersten Test nichts gemeldet, daher zusaetzlich das
    // Host-Fenster selbst pruefen.
    auto* peer = getPeer();
    const bool showing = isShowing()
                         && NoniusProbeProcessor::isNativeRootWindowVisible (peer != nullptr ? peer->getNativeHandle() : nullptr);

    if (showing != wasShowing)
    {
        processor.logEditorVisibility (showing);
        wasShowing = showing;
    }

    auto name = processor.getTrackName();
    info.setText ("Spur: " + (name.isNotEmpty() ? name : juce::String ("(noch nicht gemeldet)")) + "\n"
                  + processor.getStatusText() + "\n\n"
                  + "Log: " + processor.getLogPath(),
                  juce::dontSendNotification);
}
