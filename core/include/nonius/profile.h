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

    // Bewertung eines kompletten Mixes / Beats gegen das Profil (M13, M14, T01/T02, Breite).
    std::vector<Finding> evaluateAgainstProfile (const TrackAnalysis& t, double sideToMid, const ReferenceProfile& p, bool isMaster);

    // Stufe eines Checks aus seiner ID (A=1, S=2, K=3, T=4, M=5, sonst 0)
    int stageOf (const std::string& id);
}
