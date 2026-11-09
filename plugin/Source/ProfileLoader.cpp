#include "ProfileLoader.h"

#include <BinaryData.h>
#include <juce_core/juce_core.h>

namespace nonius
{
    namespace
    {
        Range toRange (const juce::var& v, int offset)
        {
            Range r;
            if (auto* a = v.getArray(); a != nullptr && a->size() >= offset + 3)
            {
                r.p10 = (double) (*a)[offset];
                r.p50 = (double) (*a)[offset + 1];
                r.p90 = (double) (*a)[offset + 2];
                r.valid = r.p90 >= r.p10;
            }
            return r;
        }

        ReferenceProfile parse (const juce::String& json)
        {
            ReferenceProfile p;
            const auto root = juce::JSON::parse (json);
            if (! root.isObject()) return p;

            if (auto* obj = root["bereiche"].getDynamicObject())
                for (const auto& nv : obj->getProperties())
                    p.ranges[nv.name.toString().toStdString()] = toRange (nv.value, 0);

            if (auto* oct = root["oktaven"].getArray())
                for (const auto& row : *oct)
                    if (auto* a = row.getArray(); a != nullptr && a->size() >= 4)
                    {
                        p.octaveCentres.push_back ((double) (*a)[0]);
                        p.octaves.push_back (toRange (row, 1));
                    }

            p.songs = (int) root["songs"];
            if (p.ranges.empty() && p.octaves.empty()) p.songs = 0;
            return p;
        }
    }

    namespace
    {
        std::shared_ptr<const ReferenceProfile> load (const char* fileName, const char* embedded, int embeddedSize)
        {
            const auto user = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                  .getChildFile ("Nonius").getChildFile (fileName);
            if (user.existsAsFile())
            {
                auto p = parse (user.loadFileAsString());
                if (! p.empty()) return std::make_shared<const ReferenceProfile> (std::move (p));
            }
            if (embedded == nullptr) return std::make_shared<const ReferenceProfile>();
            return std::make_shared<const ReferenceProfile> (parse (juce::String::fromUTF8 (embedded, embeddedSize)));
        }
    }

    std::shared_ptr<const ReferenceProfile> standardProfile()
    {
        static const auto profile = load ("standard-profil.json", BinaryData::standardprofil_json, BinaryData::standardprofil_jsonSize);
        return profile;
    }

    std::shared_ptr<const ReferenceProfile> vocalProfile()
    {
       #if NONIUS_HAS_VOCAL_PROFILE
        static const auto profile = load ("vocal-profil.json", BinaryData::vocalprofil_json, BinaryData::vocalprofil_jsonSize);
       #else
        static const auto profile = load ("vocal-profil.json", nullptr, 0);
       #endif
        return profile;
    }
}
