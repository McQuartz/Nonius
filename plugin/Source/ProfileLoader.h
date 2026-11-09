// Lädt das Standard-Profil (Zielbereiche aus den Referenz-Songs) einmal für alle Instanzen.
#pragma once

#include "nonius/profile.h"
#include <memory>

namespace nonius
{
    // Reihenfolge: Dokumente/Nonius/standard-profil.json, sonst das eingebaute Profil.
    std::shared_ptr<const ReferenceProfile> standardProfile();

    // Vocal-Zielwerte aus den Referenz-Stems: Dokumente/Nonius/vocal-profil.json, sonst eingebaut (falls vorhanden).
    // Leeres Profil, wenn es keins gibt; dann gelten die Startwerte aus dem Hinweis-Katalog.
    std::shared_ptr<const ReferenceProfile> vocalProfile();
}
