// Lädt das Standard-Profil (Zielbereiche aus den Referenz-Songs) einmal für alle Instanzen.
#pragma once

#include "nonius/profile.h"
#include <memory>

namespace nonius
{
    // Reihenfolge: Dokumente/Nonius/standard-profil.json, sonst das eingebaute Profil.
    std::shared_ptr<const ReferenceProfile> standardProfile();
}
