#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProfileLoader.h"

NoniusProcessor::NoniusProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      live (nonius::standardProfile())
{
}

bool NoniusProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void NoniusProcessor::prepareToPlay (double sampleRate, int)
{
    live.prepare (sampleRate, getTotalNumInputChannels());
}

void NoniusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Ton bleibt unverändert. Gemessen wird nur bei laufender Wiedergabe in Echtzeit
    // (beim Export rendert Ableton offline, das würde die Messung nur verfälschen).
    bool play = true;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            play = pos->getIsPlaying();
    if (isNonRealtime()) play = false;
    playing.store (play, std::memory_order_relaxed);

    const int numIn = getTotalNumInputChannels();
    if (play && numIn > 0)
        live.push (buffer.getArrayOfReadPointers(), numIn, buffer.getNumSamples());
}

// ------------------------------------------------------------------ Rolle / Spur
nonius::PluginRole NoniusProcessor::guessRole (const juce::String& name, nonius::PluginRole fallback)
{
    const auto n = name.toLowerCase();
    auto any = [&] (std::initializer_list<const char*> words)
    {
        for (auto* w : words) if (n.contains (w)) return true;
        return false;
    };
    if (any ({ "master", "summe", "mix bus", "mixbus", "2bus", "2-bus" })) return nonius::PluginRole::sum;
    if (any ({ "beat", "instrumental", "inst", "riddim" }))               return nonius::PluginRole::beat;
    if (any ({ "dry", "roh", "raw", "rec", "aufnahme" }))                  return nonius::PluginRole::rawVocal;
    if (any ({ "vox", "vocal", "voc", "lead", "rap", "stimme", "gesang" })) return nonius::PluginRole::leadVocal;
    return fallback;
}

void NoniusProcessor::setRole (nonius::PluginRole r, bool chosenByUser)
{
    if (chosenByUser) roleChosen = true;
    if (r != live.getRole()) live.setRole (r);
}

void NoniusProcessor::updateTrackProperties (const TrackProperties& p)
{
    {
        const juce::ScopedLock l (trackLock);
        // Name und Farbe kommen in Live teils getrennt: nur überschreiben, was gemeldet wird
        if (p.name.has_value()) trackName = *p.name;
        if (p.colourARGB.has_value()) trackColour = juce::Colour (*p.colourARGB);
    }
    // Rolle aus dem Spurnamen raten, solange du sie nicht selbst gewählt hast
    if (! roleChosen) setRole (guessRole (getTrackName(), live.getRole()), false);
}

juce::String NoniusProcessor::getTrackName() const
{
    const juce::ScopedLock l (trackLock);
    return trackName;
}

juce::Colour NoniusProcessor::getTrackColour() const
{
    const juce::ScopedLock l (trackLock);
    return trackColour;
}

// ------------------------------------------------------------------ Zustand im Projekt
void NoniusProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree state ("NONIUS");
    state.setProperty ("version", 1, nullptr);
    state.setProperty ("role", (int) getRole(), nullptr);
    state.setProperty ("roleChosen", roleChosen.load(), nullptr);
    state.setProperty ("stage", getStage(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void NoniusProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        const auto state = juce::ValueTree::fromXml (*xml);
        if (! state.hasType ("NONIUS")) return;
        roleChosen = (bool) state.getProperty ("roleChosen", false);
        live.setRole ((nonius::PluginRole) juce::jlimit (0, 3, (int) state.getProperty ("role", 0)));
        setStage ((int) state.getProperty ("stage", 5));
    }
}

juce::AudioProcessorEditor* NoniusProcessor::createEditor() { return new NoniusEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoniusProcessor(); }
