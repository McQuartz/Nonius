// NONIUS Mess-Kern: Messungen einer Spur und Vergleich zweier Spuren.
#pragma once

#include "nonius/dsp.h"

#include <string>
#include <vector>

namespace nonius
{
    struct AudioBuffer
    {
        double sampleRate = 48000.0;
        std::vector<std::vector<float>> channels; // 1 oder 2 Kanäle, gleiche Länge
        bool lossySource = false;                  // z. B. MP3: Clipping/Höhen nur eingeschränkt messbar

        std::size_t numSamples() const { return channels.empty() ? 0 : channels[0].size(); }
        double durationSeconds() const { return (double) numSamples() / sampleRate; }
        std::vector<float> mono() const;
    };

    enum class Role { leadVocal, vocalGroup, beat, sum };

    // ------------------------------------------------------------------ Einzel-Spur
    struct LoudnessResult
    {
        double integratedLufs = -150;
        double lra = 0;                    // Loudness Range (LU)
        double shortTermP10 = -150, shortTermP90 = -150;
        double maxMomentary = -150;
        std::vector<double> shortTerm;     // 3-s-Fenster, Schritt 1 s (LUFS)
        std::vector<double> momentary;     // 400-ms-Fenster, Schritt 100 ms (LUFS)
    };

    LoudnessResult measureLoudness (const AudioBuffer& audio);
    double measureTruePeakDb (const AudioBuffer& audio); // 4x Oversampling

    struct SpectrumResult
    {
        std::vector<Band> bands;            // 1/6 Oktave
        std::vector<double> levelDb;        // Langzeit-Mittel über aktive Frames, normiert (Summe 200 Hz–5 kHz = 0 dB)
        std::vector<double> p10Db, p90Db;   // Zeitstreuung pro Band, gleiche Normierung je Frame
        double tiltDbPerOct = 0;            // Neigung 100 Hz–10 kHz
        int activeFrames = 0;
    };

    struct EventStats
    {
        int count = 0;
        double perMinute = 0;
        std::vector<double> timesSeconds;
    };

    struct ResonanceHit
    {
        double frequency = 0;
        double heightDb = 0;   // über geglättetem Spektrum
        double persistence = 0; // Anteil aktiver Frames (0..1)
    };

    struct TrackAnalysis
    {
        std::string name;
        Role role = Role::leadVocal;
        double durationSeconds = 0;
        double sampleRate = 0;
        bool lossySource = false;
        int channels = 1;

        double samplePeakDb = -150;
        double truePeakDb = -150;
        double dcOffsetPercent = 0;
        double activeSeconds = 0;            // Zeit mit Signal (Stimme/Instrument)
        double activePeakDb = -150;          // Peak in aktiven Teilen
        double hotFramePercent = 0;          // Anteil 100-ms-Frames mit Peak > -3 dBFS
        int clippingRuns = 0;                // flache Spitzen (>= 3 gleiche Samples nahe Maximum)
        double noiseFloorDbA = -150;         // A-bewertet, ruhigste Pausen
        bool noiseFloorReliable = false;     // genug Pausen vorhanden
        double humExcessDb = 0;              // 50-Hz-Reihe über Nachbarn in Pausen
        double humFrequency = 0;

        LoudnessResult loudness;
        double crest100msDb = 0;             // Median über aktive Fenster
        double crest1sDb = 0;
        EventStats wordPeaks;                // K03
        EventStats plosives;                 // A07

        SpectrumResult spectrum;
        std::vector<ResonanceHit> resonances; // S03
        double rumbleDb = -150;              // Energie < 80 Hz relativ zu 200 Hz–5 kHz
        double proximityDb = -150;           // 100–250 Hz relativ zu 200 Hz–5 kHz
        double airDb = -150;                 // > 10 kHz relativ

        // Zischlaute
        EventStats sibilants;
        double sibilantToVowelDb = -150;     // Pegel S-Ereignisse relativ zu Vokalen (Median)
        double sibilantFrequency = 0;        // Schwerpunkt der S-Energie

        // Raum
        double reverbSeconds = 0;            // blinde Schätzung, Rohwert (steilste Abfälle)
        double reverbSecondsCloseMic = 0;    // grob umgerechnet auf echte RT60 bei Mikro 15–30 cm
        double reverbConfidence = 0;
        int reverbDecays = 0;
        double roomTailDb = -150;            // Pegel 60–120 ms nach Wortende relativ zum Wort (Raumanteil)
        int roomTailCount = 0;

        // Stereo
        double correlation = 1;
        double balanceDb = 0;                // L minus R (dB, aktive Teile)
        double sideBelow120Db = -150;        // Seite relativ zu Mitte unter 120 Hz
        double monoLossMaxDb = 0;            // größter Pegelverlust in Mono pro Oktavband
        double monoLossFrequency = 0;
    };

    TrackAnalysis analyseTrack (const AudioBuffer& audio, const std::string& name, Role role);

    // ------------------------------------------------------------------ Zwei Spuren
    // Vorher/Nachher (z. B. trockene gegen bearbeitete Vocal-Spur), auf gleiche Lautheit normiert.
    struct ProcessingDiff
    {
        double loudnessChangeLu = 0;
        double lraChangeLu = 0;
        double crest100msChangeDb = 0;
        double crest1sChangeDb = 0;
        double sibilantChangeDb = 0;
        double noiseChangeDb = 0;
        std::vector<Band> octaveBands;
        std::vector<double> octaveChangeDb;  // Klangänderung bei gleicher Lautheit
        double estimatedGainReductionDb = 0; // grobe Schätzung aus Short-term-Streuung
    };

    ProcessingDiff compareProcessing (const TrackAnalysis& before, const TrackAnalysis& after);

    // Vocal gegen Beat (M01–M03). Zeitachse: beide Dateien ab Sekunde 0 übereinander.
    struct MaskedRegion
    {
        double lowHz = 0, highHz = 0;
        double maskedFraction = 0;           // Anteil aktiver Vocal-Frames, in denen das Band verdeckt ist
        double neededDb = 0;                 // typischer Abstand unter der Mithörschwelle (Median)
    };

    struct MixRelation
    {
        double vocalToBeatDb = 0;            // A-bewertet, 1-s-Fenster, Median über aktive Fenster
        double vocalToBeatP10 = 0, vocalToBeatP90 = 0;
        int windowsUsed = 0;
        double overlapSeconds = 0;
        std::vector<Band> barkBandsUsed;
        std::vector<double> maskedFractionPerBark;
        std::vector<MaskedRegion> maskedRegions;
        double vocalLowMidDb = -150;         // M04: Vocal-Energie 100–250 Hz relativ
        double beatLowMidActive = 0;         // Anteil Zeit, in der der Beat dort aktiv ist
        double sumTruePeakDb = -150;         // Vocal + Beat summiert (ohne Master-Bearbeitung)
        double sumIntegratedLufs = -150;
    };

    MixRelation analyseMix (const AudioBuffer& vocal, const AudioBuffer& beat);

    // ------------------------------------------------------------------ Hinweise
    enum class Severity { ok, warning, problem, noData, info };

    struct Finding
    {
        std::string id;          // Katalog-ID, z. B. "A06"
        std::string title;       // Check-Name
        Severity severity = Severity::ok;
        std::string text;        // Hinweis-Text mit Zahlen
        std::string fix;         // Vorschlag
        std::string measured;    // Messwert (für die Tabelle)
        double priority = 0;
        std::string note;        // Einschränkung (z. B. "MP3", "experimentell")
    };

    std::vector<Finding> evaluateVocal (const TrackAnalysis& t, bool isRawRecording);
    std::vector<Finding> evaluateBeat (const TrackAnalysis& t);
    std::vector<Finding> evaluateMix (const MixRelation& m, const TrackAnalysis& vocal, const TrackAnalysis& beat);

    const char* severityLabel (Severity s);
    const char* roleLabel (Role r);
}
