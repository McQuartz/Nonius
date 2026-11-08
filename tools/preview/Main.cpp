// nonius-preview --datei song.mp3 --rolle beat|vocal|summe [--start 30] [--spur "Name"] [--stufe 5] --bild out.png
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
    struct FakePlayHead : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setIsPlaying (true);
            return p;
        }
    };
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    juce::StringPairArray args;
    for (int i = 1; i + 1 < argc; i += 2)
        args.set (juce::String (argv[i]).trimCharactersAtStart ("-"), juce::String::fromUTF8 (argv[i + 1]));

    const juce::File file (args["datei"]);
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr) { std::cerr << "Datei nicht lesbar\n"; return 1; }

    NoniusProcessor proc;
    FakePlayHead head;
    proc.setPlayHead (&head);
    const int numCh = juce::jmin (2, (int) reader->numChannels);
    proc.setPlayConfigDetails (numCh, numCh, reader->sampleRate, 512);
    proc.prepareToPlay (reader->sampleRate, 512);

    juce::AudioProcessor::TrackProperties tp;
    tp.name = args.getValue ("spur", file.getFileNameWithoutExtension());
    tp.colourARGB = 0xffe0a040u;
    proc.updateTrackProperties (tp);

    const auto rolle = args.getValue ("rolle", "summe");
    proc.setRole (rolle == "vocal" ? nonius::PluginRole::leadVocal : rolle == "beat" ? nonius::PluginRole::beat : nonius::PluginRole::sum, true);
    proc.setStage (args.getValue ("stufe", "5").getIntValue());

    // 30 s ab --start durchspielen, etwa 15-fach schneller als Echtzeit
    const auto sr = reader->sampleRate;
    const juce::int64 start = (juce::int64) (args.getValue ("start", "30").getDoubleValue() * sr);
    const juce::int64 len = juce::jmin ((juce::int64) (30.0 * sr), reader->lengthInSamples - start);
    juce::AudioBuffer<float> block (numCh, 512);
    juce::MidiBuffer midi;
    for (juce::int64 pos = 0; pos < len; pos += 512)
    {
        const int n = (int) juce::jmin ((juce::int64) 512, len - pos);
        block.setSize (numCh, n, false, false, true);
        reader->read (&block, 0, n, start + pos, true, numCh > 1);
        proc.processBlock (block, midi);
        if ((pos / 512) % 6 == 0) juce::Thread::sleep (2);
    }

    // auf die Auswertung des vollen Fensters warten
    for (int i = 0; i < 200; ++i)
    {
        juce::Thread::sleep (100);
        auto s = proc.analyser().latest();
        if (s->valid && s->secondsMeasured > 29.0) break;
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    juce::File out (args.getValue ("bild", "preview.png"));
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);

    for (const auto& f : proc.analyser().latest()->findings)
        std::cout << f.id << " | " << nonius::severityLabel (f.severity) << " | " << f.text << " | " << f.fix << "\n";
    ed.reset();
    return 0;
}
