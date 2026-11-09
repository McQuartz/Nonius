// NONIUS Mess-Kern: Zielbereiche aus Referenzen ("Standard"-Profil) und Bewertung dagegen.
#pragma once

#include "nonius/analysis.h"

#include <map>
#include <string>
#include <vector>

namespace nonius
{
    struct Range
    {
        double p10 = 0, p50 = 0, p90 = 0;
        bool valid = false;
    };

    // Zielbereiche für komplette Mixe (aus tools/reference, Datei standard-profil.json).
    struct ReferenceProfile
    {
        int songs = 0;
        std::map<std::string, Range> ranges;  // Schlüssel wie in standard-profil.json, z. B. "Lautheit integriert"
        std::vector<double> octaveCentres;    // 31.5 ... 16k
        std::vector<Range> octaves;           // dB relativ zur Gesamtleistung
        bool empty() const { return songs == 0; }
    };

    // Klangbalance einer Spur in Oktaven bzw. 4 Bändern, dB relativ zur Gesamtleistung
    // (gleiche Normierung wie das Profil).
    std::vector<double> octaveBalance (const TrackAnalysis& t, const std::vector<double>& centres);
    struct FourBands { double low = 0, lowMid = 0, highMid = 0, high = 0; };
    FourBands fourBandBalance (const TrackAnalysis& t);
    double sideToMidDb (const AudioBuffer& audio);

    // Abstand Wort zu Lücke: P95 minus P10 der 20-ms-Pegel (Leistung über alle Kanäle),
    // digitale Stille / lange Pausen (> 60 dB unter P95) ausgeklammert. 0 = zu wenig Material.
    // Klein = Lücken gefüllt (Hall, Delay, Doppler, Kompression), groß = trocken und offen.
    double wordGapDepthDb (const AudioBuffer& audio);

    // Silben-Dynamik: Verteilung der 50-ms-Pegel in aktiven Teilen (über P95 − 25 dB).
    // Kompression rückt laute und mittlere Silben zusammen; das zeigt sich hier direkt,
    // in der 3-s-Short-term-Lautheit bei dichtem Rap dagegen kaum.
    struct VocalDynamics
    {
        double syllableDb = 0;     // P90 − P30 ("Silben-Dynamik"), 0 = zu wenig Material
        double syllablePeakDb = 0; // P95 − P50 ("Silben-Spitzen")
        double gapDepthDb = 0;     // wordGapDepthDb
    };
    VocalDynamics measureVocalDynamics (const AudioBuffer& audio);

    // Vocal gegen das Vocal-Profil (vocal-profil.json, gleiches Format wie standard-profil.json;
    // Werte aus HTDemucs-Stems der Referenzen, also fertig bearbeitete Vocals).
    //  isRaw = true:  Rohaufnahme. Befunde sind der Weg zum Ziel: Kompression in dB GR, EQ pro Oktave,
    //                 De-Esser. Hall/Breite werden nicht bewertet (kommen erst später dazu).
    //  isRaw = false: bearbeitete Spur. Befunde nur, wo sie außerhalb P10–P90 liegt.
    // IDs: K01/K02 Kompression, T13 Klangkurve (eine pro Oktave, frequency gesetzt), S01 Low Cut,
    //      T06/T07 Zischlaute, M15 Lücken-Füllung (Hall/Delay), M07 Breite.
    std::vector<Finding> evaluateVocalAgainstProfile (const TrackAnalysis& t, const VocalDynamics& d,
                                                      double sideToMid, const ReferenceProfile& vocalProfile, bool isRaw);

    // evaluateVocal + evaluateVocalAgainstProfile zusammengeführt: Mit Profil ersetzen die Profil-Befunde
    // die Startwert-Checks K01, K02, T01, T06, T07, S02, T08 (ohne Profil bleibt evaluateVocal unverändert).
    std::vector<Finding> evaluateVocalWithProfile (const TrackAnalysis& t, const VocalDynamics& d,
                                                   double sideToMid, const ReferenceProfile& vocalProfile, bool isRaw);

    // Bewertung eines kompletten Mixes / Beats gegen das Profil (M13, M14, T01/T02, Breite).
    std::vector<Finding> evaluateAgainstProfile (const TrackAnalysis& t, double sideToMid, const ReferenceProfile& p, bool isMaster);

    // Stufe eines Checks aus seiner ID (A=1, S=2, K=3, T=4, M=5, sonst 0)
    int stageOf (const std::string& id);
}
