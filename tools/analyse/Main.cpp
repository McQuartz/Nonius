// nonius-analyse: Bounces reinziehen, Bericht bekommen (Phase 1).
//
// Aufruf:
//   nonius-analyse --name "Boom Bap" --raw vocal_dry.wav --vocal vocal_wet.wav [--doubles doubles.wav]
//                  --beat beat.wav --out bericht.md
#include <juce_audio_formats/juce_audio_formats.h>

#include "nonius/analysis.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <cmath>
#include <optional>

using namespace nonius;

namespace
{
    std::optional<AudioBuffer> loadAudio (const juce::File& file, juce::String& error)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr)
        {
            error = "Kann Datei nicht lesen: " + file.getFullPathName();
            return std::nullopt;
        }

        const auto len = (int) reader->lengthInSamples;
        const int chs = (int) std::min<unsigned int> (2, reader->numChannels);
        juce::AudioBuffer<float> buf (chs, len);
        reader->read (&buf, 0, len, 0, true, chs > 1);

        AudioBuffer a;
        a.sampleRate = reader->sampleRate;
        a.lossySource = file.hasFileExtension ("mp3;ogg;m4a;aac");

        // Stereo mit identischen Kanälen als Mono behandeln (Lautheit wie im Ableton-Meter bleibt gleich,
        // weil beide Kanäle gezählt werden; Stereo-Checks entfallen dann).
        bool identical = chs == 2;
        if (identical)
            for (int i = 0; i < len && identical; ++i)
                identical = std::abs (buf.getSample (0, i) - buf.getSample (1, i)) < 1e-6f;

        for (int c = 0; c < chs; ++c)
            a.channels.emplace_back (buf.getReadPointer (c), buf.getReadPointer (c) + len);
        if (identical)
            a.channels.resize (1);
        return a;
    }

    juce::String num (double v, int decimals = 1) { return juce::String (v, decimals); }

    juce::String findingRows (const std::vector<Finding>& fs)
    {
        juce::String s;
        s << "| ID | Check | Status | Befund | Vorschlag | Messwert |\n|---|---|---|---|---|---|\n";
        for (const auto& f : fs)
        {
            juce::String status (severityLabel (f.severity));
            if (f.severity == Severity::problem || f.severity == Severity::warning)
                status = "**" + status + "**";
            juce::String meas (f.measured);
            if (! f.note.empty())
                meas << " _(" << f.note << ")_";
            s << "| " << f.id << " | " << f.title << " | " << status << " | " << f.text << " | "
              << (f.fix.empty() ? juce::String::fromUTF8 ("–") : juce::String (f.fix)) << " | " << meas << " |\n";
        }
        return s;
    }

    juce::String trackFacts (const TrackAnalysis& t)
    {
        juce::String s;
        s << "| Messwert | " << t.name << " |\n|---|---|\n"
          << "| Länge / aktiv | " << num (t.durationSeconds, 1) << " s / " << num (t.activeSeconds, 1) << " s |\n"
          << "| Format | " << num (t.sampleRate / 1000.0, 1) << " kHz, " << t.channels << (t.channels == 1 ? " Kanal" : " Kanäle")
          << (t.lossySource ? ", MP3" : "") << " |\n"
          << "| Lautheit integriert | " << num (t.loudness.integratedLufs) << " LUFS |\n"
          << "| Loudness Range | " << num (t.loudness.lra) << " LU |\n"
          << "| Short-term P10–P90 | " << num (t.loudness.shortTermP10) << " bis " << num (t.loudness.shortTermP90) << " LUFS |\n"
          << "| Sample Peak / True Peak | " << num (t.samplePeakDb) << " dBFS / " << num (t.truePeakDb) << " dBTP |\n"
          << "| Crest 100 ms / 1 s | " << num (t.crest100msDb) << " dB / " << num (t.crest1sDb) << " dB |\n"
          << "| Grundrauschen (A) | " << (t.noiseFloorReliable ? (t.noiseFloorDbA <= -149 ? juce::String ("digitale Stille") : num (t.noiseFloorDbA) + " dBFS") : juce::String::fromUTF8 ("–")) << " |\n"
          << "| Spektrale Neigung | " << num (t.spectrum.tiltDbPerOct) << " dB/Okt. |\n"
          << "| < 80 Hz / 100–250 Hz / > 10 kHz | " << num (t.rumbleDb) << " / " << num (t.proximityDb) << " / " << num (t.airDb) << " dB (relativ zu 200 Hz–5 kHz) |\n"
          << "| Zischlaute | " << t.sibilants.count << " (" << num (t.sibilants.perMinute) << "/min), " << num (t.sibilantToVowelDb) << " dB zu Vokalen |\n"
          << "| Nachhall-Schätzung | " << (t.reverbDecays >= 10 ? num (t.reverbSeconds, 2) + " s (umgerechnet ca. " + num (t.reverbSecondsCloseMic, 1) + " s) aus " + juce::String (t.reverbDecays) + " Abfällen" : juce::String::fromUTF8 ("–")) << " |\n";
        if (t.channels >= 2)
            s << "| Korrelation / Balance | " << num (t.correlation, 2) << " / " << num (t.balanceDb) << " dB |\n";
        return s;
    }

    juce::String octaveTable (const TrackAnalysis& t)
    {
        // 1/6-Oktav-Spektrum zu Oktaven zusammengefasst
        juce::String head = "| Band |", sep = "|---|", row = "| " + juce::String (t.name) + " |";
        auto oct = fractionalOctaveBands (1, 31.5, std::min (16000.0, t.sampleRate * 0.45));
        for (const auto& o : oct)
        {
            double p = 0;
            for (std::size_t i = 0; i < t.spectrum.bands.size(); ++i)
                if (t.spectrum.bands[i].centre >= o.low && t.spectrum.bands[i].centre < o.high)
                    p += std::pow (10.0, t.spectrum.levelDb[i] / 10.0);
            head << " " << (o.centre >= 1000 ? juce::String (o.centre / 1000.0, 0) + "k" : juce::String ((int) std::round (o.centre))) << " |";
            sep << "---|";
            row << " " << num (toDb (p)) << " |";
        }
        return head + "\n" + sep + "\n" + row + "\n";
    }
}

int main (int argc, char* argv[])
{
    std::map<juce::String, juce::String> args;
    for (int i = 1; i + 1 < argc; i += 2)
        args[juce::String (argv[i]).trimCharactersAtStart ("-")] = juce::String::fromUTF8 (argv[i + 1]);

    if (! args.count ("vocal") || ! args.count ("beat"))
    {
        std::printf ("Aufruf: nonius-analyse --name NAME [--raw ROH] --vocal FERTIG [--doubles DOUBLES] --beat BEAT [--out BERICHT.md]\n");
        return 1;
    }

    const juce::String name = args.count ("name") ? args["name"] : juce::String ("Song");
    juce::String err;

    auto load = [&] (const juce::String& key) -> std::optional<AudioBuffer>
    {
        if (! args.count (key)) return std::nullopt;
        auto a = loadAudio (juce::File (args[key]), err);
        if (! a) std::fprintf (stderr, "%s\n", err.toRawUTF8());
        return a;
    };

    auto raw = load ("raw");
    auto vocal = load ("vocal");
    auto doubles = load ("doubles");
    auto beat = load ("beat");
    if (! vocal || ! beat) return 2;

    std::printf ("Analysiere %s ...\n", name.toRawUTF8());
    std::optional<TrackAnalysis> tRaw;
    if (raw) tRaw = analyseTrack (*raw, "Vocal roh", Role::leadVocal);
    auto tVocal = analyseTrack (*vocal, "Vocal fertig", Role::leadVocal);
    std::optional<TrackAnalysis> tDoubles;
    if (doubles) tDoubles = analyseTrack (*doubles, "Doubles", Role::vocalGroup);
    auto tBeat = analyseTrack (*beat, "Beat", Role::beat);
    auto mix = analyseMix (*vocal, *beat);

    std::vector<Finding> fRaw;
    if (tRaw) fRaw = evaluateVocal (*tRaw, true);
    auto fVocal = evaluateVocal (tVocal, false);
    auto fBeat = evaluateBeat (tBeat);
    auto fMix = evaluateMix (mix, tVocal, tBeat);

    // Top-Hinweise nach Priorität
    struct Ranked { const Finding* f; juce::String where; };
    std::vector<Ranked> ranked;
    auto collect = [&] (const std::vector<Finding>& fs, const juce::String& where)
    {
        for (const auto& f : fs)
            if (f.severity == Severity::warning || f.severity == Severity::problem)
                ranked.push_back ({ &f, where });
    };
    collect (fRaw, "Rohspur"); collect (fVocal, "Vocal fertig"); collect (fMix, "Vocal gegen Beat"); collect (fBeat, "Beat");
    std::sort (ranked.begin(), ranked.end(), [] (const Ranked& a, const Ranked& b)
    {
        const int sa = a.f->severity == Severity::problem ? 1 : 0, sb = b.f->severity == Severity::problem ? 1 : 0;
        return sa != sb ? sa > sb : a.f->priority > b.f->priority;
    });

    juce::String md;
    md << "# NONIUS Bericht: " << name << "\n\n"
       << "_Phase-1-Testprogramm, Hinweis-Katalog v0.1. Grenzwerte sind Startwerte, noch ohne deine Referenzen._\n\n"
       << "Dateien: ";
    if (args.count ("raw")) md << "Roh `" << juce::File (args["raw"]).getFileName() << "`, ";
    md << "Fertig `" << juce::File (args["vocal"]).getFileName() << "`, ";
    if (args.count ("doubles")) md << "Doubles `" << juce::File (args["doubles"]).getFileName() << "`, ";
    md << "Beat `" << juce::File (args["beat"]).getFileName() << "`\n\n";

    md << "## Die wichtigsten Punkte\n\n";
    if (ranked.empty()) md << "Keine Auffälligkeiten nach den aktuellen Startwerten.\n";
    for (std::size_t i = 0; i < std::min<std::size_t> (5, ranked.size()); ++i)
    {
        const auto& f = *ranked[i].f;
        md << (int) (i + 1) << ". **" << f.text << "** (" << ranked[i].where << ", " << f.id << ")";
        if (! f.fix.empty()) md << " → " << f.fix;
        md << "\n";
    }
    md << "\n";

    if (tRaw)
        md << "## 1. Rohspur: Aufnahme\n\n" << findingRows (fRaw) << "\n";

    md << "## 2. Fertige Vocal-Spur\n\n" << findingRows (fVocal) << "\n";

    if (tRaw)
    {
        auto d = compareProcessing (*tRaw, tVocal);
        md << "## 3. Was deine Bearbeitung verändert hat (roh → fertig)\n\n"
           << "| Größe | Änderung |\n|---|---|\n"
           << "| Lautheit | " << num (d.loudnessChangeLu) << " LU |\n"
           << "| Loudness Range | " << num (d.lraChangeLu) << " LU |\n"
           << "| Streuung Short-term (≈ Kompression) | −" << num (d.estimatedGainReductionDb) << " LU |\n"
           << "| Crest 100 ms / 1 s | " << num (d.crest100msChangeDb) << " / " << num (d.crest1sChangeDb) << " dB |\n"
           << "| Zischlaute relativ zu Vokalen | " << num (d.sibilantChangeDb) << " dB |\n"
           << "| Rauschen relativ zur Stimme | " << num (d.noiseChangeDb) << " dB |\n\n"
           << "Klangänderung bei gleicher Lautheit (Oktavbänder, dB):\n\n";
        juce::String h = "|", s = "|", r = "|";
        for (std::size_t i = 0; i < d.octaveBands.size(); ++i)
        {
            const double c = d.octaveBands[i].centre;
            h << " " << (c >= 1000 ? juce::String (c / 1000.0, 0) + "k" : juce::String ((int) std::round (c))) << " |";
            s << "---|";
            r << " " << (d.octaveChangeDb[i] >= 0 ? "+" : "") << num (d.octaveChangeDb[i]) << " |";
        }
        md << h << "\n" << s << "\n" << r << "\n\n";
    }

    md << "## 4. Vocal gegen Beat\n\n" << findingRows (fMix) << "\n"
       << "Verdeckung pro Bark-Band (Anteil der Zeit, in der das Vocal unter der Mithörschwelle des Beats liegt):\n\n";
    {
        juce::String h = "|", s = "|", r = "|";
        for (std::size_t i = 0; i < mix.barkBandsUsed.size(); ++i)
        {
            const double c = mix.barkBandsUsed[i].centre;
            h << " " << (c >= 1000 ? juce::String (c / 1000.0, 1) + "k" : juce::String ((int) std::round (c))) << " |";
            s << "---|";
            r << " " << juce::String ((int) std::round (mix.maskedFractionPerBark[i] * 100)) << " % |";
        }
        md << h << "\n" << s << "\n" << r << "\n\n";
    }
    if (std::abs (vocal->durationSeconds() - beat->durationSeconds()) > 1.0)
        md << "_Hinweis: Vocal (" << num (vocal->durationSeconds()) << " s) und Beat (" << num (beat->durationSeconds())
           << " s) sind unterschiedlich lang. Ausgewertet wird ab Sekunde 0, beide Dateien müssen also am selben Punkt beginnen._\n\n";

    if (tDoubles)
    {
        const double rel = tDoubles->loudness.integratedLufs - tVocal.loudness.integratedLufs;
        md << "## 5. Doubles\n\nDoubles liegen " << num (rel) << " LU " << (rel < 0 ? "unter" : "über") << " dem Lead (M06: Hinweis ab −6 LU)"
           << (rel > -6.0 ? " → **ACHTUNG: Doubles fast so laut wie der Lead**" : "") << ".\n\n";
    }

    md << "## " << (tDoubles ? "6" : "5") << ". Beat\n\n" << findingRows (fBeat) << "\n";

    // Stellen zum Nachhören (für die Kalibrierung: stimmt der Befund mit dem Gehör überein?)
    auto times = [] (const EventStats& e)
    {
        juce::String s;
        for (std::size_t i = 0; i < std::min<std::size_t> (12, e.timesSeconds.size()); ++i)
        {
            const int sec = (int) e.timesSeconds[i];
            s << (i ? ", " : "") << juce::String (sec / 60) << ":" << juce::String (sec % 60).paddedLeft ('0', 2);
        }
        if (e.timesSeconds.size() > 12) s << " …";
        return s.isEmpty() ? juce::String::fromUTF8 ("–") : s;
    };
    md << "## Stellen zum Nachhören\n\n"
       << "Hör bitte bei ein paar dieser Stellen rein und sag mir, ob der Befund stimmt. Damit stelle ich die Grenzwerte ein.\n\n"
       << "| Was | Spur | Zeitpunkte (m:ss) |\n|---|---|---|\n";
    if (tRaw)
        md << "| Popps | Vocal roh | " << times (tRaw->plosives) << " |\n"
           << "| Wortspitzen | Vocal roh | " << times (tRaw->wordPeaks) << " |\n";
    md << "| Wortspitzen | Vocal fertig | " << times (tVocal.wordPeaks) << " |\n"
       << "| Zischlaute | Vocal fertig | " << times (tVocal.sibilants) << " |\n\n";

    md << "## Messwerte im Detail\n\n";
    if (tRaw) md << trackFacts (*tRaw) << "\n";
    md << trackFacts (tVocal) << "\n";
    if (tDoubles) md << trackFacts (*tDoubles) << "\n";
    md << trackFacts (tBeat) << "\n";
    md << "Spektrum (Oktaven, dB relativ zu 200 Hz–5 kHz):\n\n";
    if (tRaw) md << octaveTable (*tRaw) << "\n";
    md << octaveTable (tVocal) << "\n" << octaveTable (tBeat) << "\n";

    md << "## Grenzen dieser Messung\n\n"
       << "- Grenzwerte sind Startwerte aus dem Katalog. Bewertungen, die ein Ziel brauchen (Mumpf, Helligkeit, Air), kommen erst mit deinen Referenzen.\n"
       << "- MP3: Clipping ist nach MP3 nicht sicher erkennbar, Höhen über ca. 16 kHz fehlen oft, die Spuren können minimal verschoben sein.\n"
       << "- Raumhall-Schätzung und Verdeckungsmodell sind noch nicht kalibriert.\n";

    const juce::File out (args.count ("out") ? args["out"] : juce::String ("nonius-bericht.md"));
    out.replaceWithText (md, false, false, "\n");
    std::printf ("Bericht: %s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
