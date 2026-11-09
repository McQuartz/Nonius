// nonius-preview --datei song.mp3 --rolle beat|vocal|summe [--start 30] [--spur "Name"] [--stufe 5] --bild out.png
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
    struct FakePlayHead : juce::AudioPlayHead
    {
        std::atomic<bool> playing { true };
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setIsPlaying (playing.load());
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
    proc.setRole (rolle == "roh" ? nonius::PluginRole::rawVocal : rolle == "vocal" ? nonius::PluginRole::leadVocal : rolle == "beat" ? nonius::PluginRole::beat : nonius::PluginRole::sum, true);
    proc.setStage (args.getValue ("stufe", "5").getIntValue());

    // --echtzeit N: N Sekunden in Echtzeit wie in Live abspielen (mit Stopp von Sekunde 20 bis 25)
    // und jede Sekunde den Zustand der Messung ausgeben. Prüft Timing, Pausen und Live-Anzeige.
    if (args.containsKey ("echtzeit"))
    {
        const double total = args["echtzeit"].getDoubleValue();
        const auto sr0 = reader->sampleRate;
        juce::AudioBuffer<float> blk (numCh, 512);
        juce::MidiBuffer m;
        juce::int64 pos = (juce::int64) (args.getValue ("start", "0").getDoubleValue() * sr0);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        int lastSec = -1;
        for (juce::int64 n = 0; (double) n / sr0 < total; n += 512)
        {
            const double t = (double) n / sr0;
            head.playing = ! (t >= 20.0 && t < 25.0);
            if (head.playing.load())
            {
                reader->read (&blk, 0, 512, pos, true, numCh > 1);
                pos += 512;
            }
            else blk.clear(); // Live liefert bei Stopp Stille
            proc.processBlock (blk, m);
            const double due = t0 + t * 1000.0;
            while (juce::Time::getMillisecondCounterHiRes() < due) juce::Thread::sleep (1);
            if ((int) t != lastSec)
            {
                lastSec = (int) t;
                auto s = proc.analyser().latest();
                int issues = 0;
                for (auto& f : s->findings) if (f.severity == nonius::Severity::warning || f.severity == nonius::Severity::problem) ++issues;
                const auto live = proc.analyser().liveOctaves();
                juce::String l;
                for (double v : live) l << juce::String (v, 1) << " ";
                std::cout << "t=" << lastSec << " play=" << (int) proc.isPlaying() << " puffer=" << juce::String (proc.analyser().bufferedSeconds(), 1)
                          << " gueltig=" << (int) s->valid << " gemessen=" << juce::String (s->secondsMeasured, 1)
                          << " befunde=" << issues << " live=[" << l << "]\n";
            }
        }
        return 0;
    }

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
