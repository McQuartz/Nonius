// nonius-vocal: misst eine Vocal-Spur (z. B. HTDemucs-Stem aus einer Referenz oder eine eigene Spur)
// und optional den Beat dazu. Ausgabe: eine JSON-Zeile mit allen Werten, die das Vocal-Profil braucht.
//
// Aufruf:
//   nonius-vocal --vocal vocals.wav [--beat no_vocals.wav] [--name NAME] [--start SEK] [--dauer SEK]
//                [--kompression DB] [--out werte.json]
// --kompression wendet vor der Messung einen einfachen Kompressor an (Kalibrierung der
// Kompressions-Schätzung): Ratio 4:1, Attack 5 ms, Release 80 ms, Schwelle so, dass die mittlere
// Gain Reduction in aktiven Teilen etwa DB beträgt.
#include <juce_audio_formats/juce_audio_formats.h>

#include "nonius/analysis.h"
#include "nonius/profile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

using namespace nonius;

namespace
{
    bool load (const juce::File& f, AudioBuffer& a, double start, double dur)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr) return false;
        const auto s0 = (juce::int64) (start * r->sampleRate);
        auto len = r->lengthInSamples - s0;
        if (dur > 0) len = std::min<juce::int64> (len, (juce::int64) (dur * r->sampleRate));
        if (len <= 0) return false;
        const int chs = (int) std::min<unsigned int> (2, r->numChannels);
        juce::AudioBuffer<float> b (chs, (int) len);
        r->read (&b, 0, (int) len, s0, true, chs > 1);
        a.sampleRate = r->sampleRate;
        a.lossySource = ! f.hasFileExtension ("wav;aif;aiff;flac");
        a.channels.clear();
        for (int c = 0; c < chs; ++c)
            a.channels.emplace_back (b.getReadPointer (c), b.getReadPointer (c) + len);
        return true;
    }

    // Feed-forward-Kompressor (Spitzen-Detektor, gemeinsame Regelung für alle Kanäle).
    double attackMs = 5.0, releaseMs = 80.0, ratioGlobal = 4.0;

    double compress (AudioBuffer& a, double thresholdDb, double ratio)
    {
        const double sr = a.sampleRate;
        const double att = std::exp (-1.0 / (attackMs * 0.001 * sr)), rel = std::exp (-1.0 / (releaseMs * 0.001 * sr));
        double env = 0, grSum = 0; std::size_t grN = 0;
        for (std::size_t i = 0; i < a.numSamples(); ++i)
        {
            double x = 0;
            for (auto& ch : a.channels) x = std::max (x, (double) std::fabs (ch[i]));
            env = x > env ? att * env + (1 - att) * x : rel * env + (1 - rel) * x;
            const double lv = 20.0 * std::log10 (std::max (env, 1e-9));
            const double gr = lv > thresholdDb ? (lv - thresholdDb) * (1.0 - 1.0 / ratio) : 0.0;
            const float g = (float) std::pow (10.0, -gr / 20.0);
            for (auto& ch : a.channels) ch[i] *= g;
            if (lv > -50.0) { grSum += gr; ++grN; }
        }
        return grN ? grSum / (double) grN : 0.0;
    }

    double rmsActiveDb (const AudioBuffer& a)
    {
        // Pegel der lauten Teile (P90 der 100-ms-Fenster), Bezug für die Kompressor-Schwelle
        const std::size_t w = (std::size_t) (a.sampleRate * 0.1);
        std::vector<double> lv;
        for (std::size_t i = 0; i + w <= a.numSamples(); i += w)
        {
            double p = 0;
            for (auto& ch : a.channels) for (std::size_t j = i; j < i + w; ++j) p += (double) ch[j] * ch[j];
            lv.push_back (toDb (p / (double) (w * a.channels.size())));
        }
        return lv.empty() ? -60.0 : percentile (lv, 90);
    }

    ReferenceProfile loadProfile (const juce::File& f)
    {
        ReferenceProfile p;
        const auto root = juce::JSON::parse (f.loadFileAsString());
        auto toRange = [] (const juce::var& v, int off)
        {
            Range r;
            if (auto* a = v.getArray(); a != nullptr && a->size() >= off + 3)
            { r.p10 = (*a)[off]; r.p50 = (*a)[off + 1]; r.p90 = (*a)[off + 2]; r.valid = r.p90 >= r.p10; }
            return r;
        };
        if (auto* obj = root["bereiche"].getDynamicObject())
            for (const auto& nv : obj->getProperties()) p.ranges[nv.name.toString().toStdString()] = toRange (nv.value, 0);
        if (auto* oct = root["oktaven"].getArray())
            for (const auto& row : *oct) { p.octaveCentres.push_back ((double) row[0]); p.octaves.push_back (toRange (row, 1)); }
        p.songs = (int) root["songs"];
        return p;
    }

    juce::var arr (const std::vector<double>& v)
    {
        juce::Array<juce::var> a;
        for (double x : v) a.add (x);
        return a;
    }
}

int main (int argc, char* argv[])
{
    std::map<juce::String, juce::String> args;
    for (int i = 1; i + 1 < argc; i += 2)
        args[juce::String (argv[i]).trimCharactersAtStart ("-")] = juce::String::fromUTF8 (argv[i + 1]);
    if (! args.count ("vocal")) { std::printf ("Aufruf: nonius-vocal --vocal v.wav [--beat b.wav] [--name N] [--start s] [--dauer s] [--kompression dB] [--out x.json]\n"); return 1; }

    const double start = args.count ("start") ? args["start"].getDoubleValue() : 0.0;
    const double dur = args.count ("dauer") ? args["dauer"].getDoubleValue() : 0.0;
    AudioBuffer v;
    if (! load (juce::File (args["vocal"]), v, start, dur)) { std::fprintf (stderr, "Vocal nicht lesbar\n"); return 2; }

    if (args.count ("attack")) attackMs = args["attack"].getDoubleValue();
    if (args.count ("release")) releaseMs = args["release"].getDoubleValue();
    if (args.count ("ratio")) ratioGlobal = args["ratio"].getDoubleValue();
    double appliedGr = 0;
    if (args.count ("kompression"))
    {
        const double target = args["kompression"].getDoubleValue();
        if (target > 0)
        {
            // Schwelle iterativ suchen, bis die mittlere Gain Reduction passt
            const double ref = rmsActiveDb (v);
            double lo = ref - 40, hi = ref + 6;
            AudioBuffer best = v;
            for (int it = 0; it < 14; ++it)
            {
                const double th = 0.5 * (lo + hi);
                AudioBuffer c = v;
                const double gr = compress (c, th, ratioGlobal);
                best = c; appliedGr = gr;
                if (gr > target) lo = th; else hi = th;
            }
            v = best;
        }
    }

    const auto name = args.count ("name") ? args["name"] : juce::File (args["vocal"]).getParentDirectory().getFileName();
    auto t = analyseTrack (v, name.toStdString(), Role::leadVocal);
    const std::vector<double> centres { 31.5, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
    const auto oct = octaveBalance (t, centres);
    const auto four = fourBandBalance (t);

    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty ("name", name);
    o->setProperty ("dauer", t.durationSeconds);
    o->setProperty ("aktivSek", t.activeSeconds);
    o->setProperty ("kompressionAngewandt", appliedGr);
    o->setProperty ("lufs", t.loudness.integratedLufs);
    o->setProperty ("lra", t.loudness.lra);
    {
        // Wort-/Zeilen-Dynamik: Momentary-Lautheit (400 ms) in aktiven Teilen (innerhalb 20 LU unter dem Maximum)
        std::vector<double> mom;
        const auto& m = t.loudness.momentary;
        double mx = -150; for (double x : m) mx = std::max (mx, x);
        for (double x : m) if (x > mx - 20.0) mom.push_back (x);
        o->setProperty ("wort90_10", mom.size() > 20 ? percentile (mom, 90) - percentile (mom, 10) : 0.0);
        o->setProperty ("wort95_50", mom.size() > 20 ? percentile (mom, 95) - percentile (mom, 50) : 0.0);
    }
    o->setProperty ("streuung", t.loudness.shortTermP90 - t.loudness.shortTermP10);
    o->setProperty ("crest100", t.crest100msDb);
    const auto vd = measureVocalDynamics (v);
    o->setProperty ("silbe95_50", vd.syllablePeakDb);
    o->setProperty ("silbe90_30", vd.syllableDb);
    o->setProperty ("crest1s", t.crest1sDb);
    o->setProperty ("wortspitzenProMin", t.wordPeaks.perMinute);
    o->setProperty ("zischS", t.sibilantToVowelDb);
    o->setProperty ("zischFreq", t.sibilantFrequency);
    o->setProperty ("zischProMin", t.sibilants.perMinute);
    o->setProperty ("luecke", vd.gapDepthDb);
    o->setProperty ("raumNachklang", t.roomTailDb);
    o->setProperty ("raumNachklangN", t.roomTailCount);
    o->setProperty ("neigung", t.spectrum.tiltDbPerOct);
    o->setProperty ("rumpeln", t.rumbleDb);
    o->setProperty ("naehe", t.proximityDb);
    o->setProperty ("luft", t.airDb);
    o->setProperty ("breite", sideToMidDb (v));
    o->setProperty ("tief", four.low);
    o->setProperty ("tiefeMitten", four.lowMid);
    o->setProperty ("hoheMitten", four.highMid);
    o->setProperty ("hoehen", four.high);
    o->setProperty ("oktaven", arr (oct));
    o->setProperty ("peakAktiv", t.activePeakDb);

    if (args.count ("beat"))
    {
        AudioBuffer b;
        if (load (juce::File (args["beat"]), b, start, dur) && b.sampleRate == v.sampleRate)
        {
            const auto m = analyseMix (v, b);
            o->setProperty ("vocalZuBeat", m.vocalToBeatDb);
            o->setProperty ("vocalZuBeatP10", m.vocalToBeatP10);
            o->setProperty ("vocalZuBeatP90", m.vocalToBeatP90);
            o->setProperty ("mixFenster", m.windowsUsed);
            juce::Array<juce::var> masked;
            for (const auto& r : m.maskedRegions)
            {
                juce::DynamicObject::Ptr x = new juce::DynamicObject();
                x->setProperty ("von", r.lowHz); x->setProperty ("bis", r.highHz);
                x->setProperty ("anteil", r.maskedFraction); x->setProperty ("db", r.neededDb);
                masked.add (juce::var (x.get()));
            }
            o->setProperty ("verdeckt", masked);
        }
        else std::fprintf (stderr, "Beat nicht lesbar oder andere Samplerate\n");
    }

    if (args.count ("profil"))
    {
        // Befunde gegen das Vocal-Profil (wie im Plugin), --roh 1 = Rohaufnahme
        const auto prof = loadProfile (juce::File (args["profil"]));
        const bool raw = args.count ("roh") && args["roh"].getIntValue() != 0;
        juce::Array<juce::var> fs;
        for (const auto& f : evaluateVocalWithProfile (t, vd, sideToMidDb (v), prof, raw))
        {
            juce::DynamicObject::Ptr x = new juce::DynamicObject();
            x->setProperty ("id", juce::String (f.id)); x->setProperty ("titel", juce::String::fromUTF8 (f.title.c_str()));
            x->setProperty ("stufe", juce::String (severityLabel (f.severity)));
            x->setProperty ("text", juce::String::fromUTF8 (f.text.c_str())); x->setProperty ("fix", juce::String::fromUTF8 (f.fix.c_str()));
            x->setProperty ("wert", juce::String::fromUTF8 (f.measured.c_str()));
            fs.add (juce::var (x.get()));
        }
        o->setProperty ("befunde", fs);
    }

    const auto json = juce::JSON::toString (juce::var (o.get()), true);
    if (args.count ("out")) juce::File (args["out"]).replaceWithText (json + "\n", false, false, "\n");
    else std::printf ("%s\n", json.toRawUTF8());
    return 0;
}
