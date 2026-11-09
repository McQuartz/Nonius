#include "PluginEditor.h"

#include "nonius/profile.h"

#include <cmath>

namespace
{
    namespace C
    {
        const juce::Colour bg      { 0xff151617 };
        const juce::Colour panel   { 0xff1a1b1d };
        const juce::Colour grid    { 0xff26272a };
        const juce::Colour grid2   { 0xff2a2b2e };
        const juce::Colour slot    { 0xff34353a };
        const juce::Colour range   { 0xff3a3b3f };
        const juce::Colour text    { 0xffecebe7 };
        const juce::Colour muted   { 0xffa19f99 };
        const juce::Colour dim     { 0xff76746f };
        const juce::Colour accent  { 0xfff2a541 };
        const juce::Colour bad     { 0xffff7752 };
        const juce::Colour ok      { 0xff8fd6a0 };
    }

    constexpr int kWidth = 1040, kHeight = 660;

    // Literale mit Umlauten/Sonderzeichen immer über UTF-8 (sonst zerlegt juce::String sie)
    juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    juce::String u8 (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }

    juce::String sansName()
    {
       #if JUCE_WINDOWS
        return "Segoe UI";
       #else
        return juce::Font::getDefaultSansSerifFontName();
       #endif
    }

    juce::String monoName()
    {
       #if JUCE_WINDOWS
        return "Consolas";
       #else
        return juce::Font::getDefaultMonospacedFontName();
       #endif
    }

    juce::Font sans (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (sansName(), h, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::Font mono (float h) { return juce::Font (juce::FontOptions (monoName(), h, juce::Font::plain)); }

    juce::Font caps (float h) { return sans (h, true).withExtraKerningFactor (0.18f); }

    juce::Colour severityColour (nonius::Severity s)
    {
        switch (s)
        {
            case nonius::Severity::problem: return C::bad;
            case nonius::Severity::warning: return C::accent;
            case nonius::Severity::ok:      return C::ok;
            case nonius::Severity::info:    return C::muted;
            case nonius::Severity::noData:  return C::dim;
        }
        return C::dim;
    }

    juce::String hzText (double f)
    {
        if (f >= 1000.0)
        {
            const double k = f / 1000.0;
            return (std::fabs (k - std::round (k)) < 0.05 ? juce::String ((int) std::round (k)) : juce::String (k, 1)) + "k";
        }
        return juce::String ((int) std::round (f));
    }

    const char* const kStageNames[5] = { "Aufnahme", u8"Säubern", "Kontrolle", "Ton", "Mix" };

    // Was misst NONIUS, und womit beeinflusst man es? (Info-Menü "Messgrößen erklärt", Tooltips der Meter)
    struct GlossaryEntry { const char* key; const char* meaning; const char* tools; };
    const GlossaryEntry kGlossary[] = {
        { "Klangbild",              "Energie pro Oktave im Vergleich zu den Referenzen (0 = Median).", "EQ (Pro-Q, EQ Eight): breite Glocken, Shelves" },
        { "Spektrale Neigung",      "Wie hell oder dunkel insgesamt (dB/Okt., negativer = dunkler).", "High-/Low-Shelf; Sättigung oder Exciter für mehr Höhen" },
        { "Silben-Dynamik",         "Abstand laute zu leise Silben. Klein = stark komprimiert.", "Kompressor (Gain Reduction, Attack), Clip-Gain" },
        { "Lautstärke-Schwankung",  "Schwankung über Sekunden, z. B. zwischen Zeilen.", "Langsamer Kompressor, Volume-Automation, Clip-Gain" },
        { "Crest",                  "Spitzen gegenüber dem Durchschnitt (100 ms bzw. 1 s). Klein = platt.", "Schneller Kompressor, Limiter oder Clipper senken ihn" },
        { "Wortspitzen",            "Einzelne Wörter, die deutlich herausspringen.", "Clip-Gain oder schneller Limiter vor dem Kompressor" },
        { "Zischlaute",             "S-Laute im Verhältnis zu den Vokalen.", "De-Esser (Pro-DS u. a.), notfalls schmaler EQ-Cut" },
        { "Abstand Wort zu Lücke",  "Wie leer die Lücken zwischen Wörtern sind. Klein = gefüllt.", "Hall/Delay füllen die Lücken, Gate/Expander leeren sie" },
        { "Breite",                 "Seitensignal im Verhältnis zur Mitte (Stereo-Breite).", "Doubles/Adlibs panen, Stereo-Effekte, Utility Width" },
        { "Bass-Seite",             "Bass unter 120 Hz in Stereo. Sollte fast mono sein.", "Utility: Bass Mono" },
        { "Bass unter 250 Hz",      "Anteil Bass am Gesamtklang.", "EQ Low-Shelf, Pegel von Kick/808" },
        { u8"Höhen über 8 kHz",     "Anteil Höhen am Gesamtklang.", "EQ High-Shelf, Exciter; zu viel: De-Esser/Shelf runter" },
        { "Rumpeln",                "Energie unter 80 Hz bei der Stimme (Trittschall, Brummen, Wind).", "Low Cut 80–100 Hz" },
        { "Rauschen",               "Grundrauschen in den Pausen (A-bewertet).", "Aufnahme: Gain/Mikro. Mix: Gate, Expander, Entrauscher" },
        { "Raumhall",               "Wie viel Raum in der Aufnahme ist (experimentell).", "Aufnahme: Absorber, näher ans Mikro. Mix: De-Reverb" },
        { "Popps",                  "Plosive (P, B), die tief einschlagen.", "Popschutz; im Mix Low Cut und Clip-Gain" },
        { "Clipping",               "Übersteuerte Stellen in der Aufnahme.", "Interface-Gain runter; Declipper" },
        { "Peaks",                  "Spitzenpegel beim Aufnehmen.", "Gain am Interface, Ziel −10 bis −6 dBFS" },
        { "True Peak",              "Echte Spitzen zwischen den Samples.", "Limiter mit Ceiling −1 dBTP" },
        { "Lautheit",               "Gesamtlautheit in LUFS (nur auf der Summe sinnvoll).", "Limiter / Mastering" },
        { "Resonanz",               "Einzelne Frequenz, die ständig hervorsticht.", "Schmaler EQ-Cut (Q 4–8) oder dynamischer EQ / Soothe" },
    };

    const GlossaryEntry* glossaryFor (const juce::String& label)
    {
        for (const auto& e : kGlossary)
            if (label.startsWith (juce::String::fromUTF8 (e.key))) return &e;
        return nullptr;
    }

    bool isIssue (const nonius::Finding& f)
    {
        return f.severity == nonius::Severity::problem || f.severity == nonius::Severity::warning;
    }
}

// ====================================================================== LookAndFeel
NoniusLookAndFeel::NoniusLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, C::bg);
    setColour (juce::ComboBox::backgroundColourId, C::panel);
    setColour (juce::ComboBox::outlineColourId, C::range);
    setColour (juce::ComboBox::textColourId, C::text);
    setColour (juce::ComboBox::arrowColourId, C::muted);
    setColour (juce::PopupMenu::backgroundColourId, C::panel);
    setColour (juce::PopupMenu::textColourId, C::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, C::slot);
    setColour (juce::PopupMenu::highlightedTextColourId, C::text);
    setColour (juce::TextButton::buttonColourId, C::bg);
    setColour (juce::TextButton::buttonOnColourId, C::slot);
    setColour (juce::TextButton::textColourOffId, C::text);
    setColour (juce::TextButton::textColourOnId, C::accent);
    setColour (juce::TooltipWindow::backgroundColourId, C::panel);
    setColour (juce::TooltipWindow::textColourId, C::text);
    setColour (juce::TooltipWindow::outlineColourId, C::range);
    setDefaultSansSerifTypefaceName (sansName());
}

void NoniusLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool filled = b.getProperties()["filled"];
    if (filled)
    {
        g.setColour (base.brighter (down ? -0.1f : over ? 0.08f : 0.0f));
        g.fillRoundedRectangle (r, 4.0f);
        return;
    }
    if (b.getToggleState()) { g.setColour (C::slot); g.fillRoundedRectangle (r, 4.0f); }
    else if (over && b.isEnabled()) { g.setColour (C::grid); g.fillRoundedRectangle (r, 4.0f); }
    g.setColour (b.getToggleState() ? C::accent : C::range);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

juce::Font NoniusLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return sans (13.0f, true); }
juce::Font NoniusLookAndFeel::getComboBoxFont (juce::ComboBox&)        { return sans (13.0f); }
juce::Font NoniusLookAndFeel::getPopupMenuFont()                        { return sans (14.0f); }

// ====================================================================== Editor
NoniusEditor::NoniusEditor (NoniusProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setOpaque (true);

    roleBox.addItem ("Vocal roh (Aufnahme)", 4);
    roleBox.addItem ("Vocal bearbeitet", 1);
    roleBox.addItem ("Beat", 2);
    roleBox.addItem ("Summe / Master", 3);
    roleBox.setSelectedId ((int) proc.getRole() + 1, juce::dontSendNotification);
    roleBox.setTooltip (u8 ("Was liegt auf dieser Spur? Danach richten sich die Checks und Zielwerte."));
    roleBox.onChange = [this]
    {
        proc.setRole ((nonius::PluginRole) (roleBox.getSelectedId() - 1), true);
        showAll = false;
        repaint();
    };
    addAndMakeVisible (roleBox);

    centralButton.setButtonText (u8 ("Zentrale ↗"));
    centralButton.setEnabled (false);
    centralButton.setTooltip (u8 ("Gesamtübersicht über alle Spuren: kommt in Phase 3"));
    addAndMakeVisible (centralButton);

    holdButton.setClickingTogglesState (true);
    holdButton.setTooltip (u8 ("Anzeige einfrieren, während weiter gemessen wird"));
    holdButton.onClick = [this]
    {
        frozen = holdButton.getToggleState();
        if (! frozen) timerCallback();
        repaint();
    };
    addAndMakeVisible (holdButton);

    measureButton.getProperties().set ("filled", true);
    measureButton.setColour (juce::TextButton::buttonColourId, C::accent);
    measureButton.setColour (juce::TextButton::textColourOffId, C::bg);
    measureButton.setTooltip (u8 ("Gemessen wird nur, solange Live abspielt. Klick: Messung neu starten (z. B. nach einer Änderung am Mix)"));
    measureButton.onClick = [this]
    {
        proc.analyser().requestReset();
        frozen = false;
        holdButton.setToggleState (false, juce::dontSendNotification);
        showAll = false;
        repaint();
    };
    addAndMakeVisible (measureButton);

    setSize (kWidth, kHeight);
    timerCallback();
    startTimerHz (25);
}

NoniusEditor::~NoniusEditor() { setLookAndFeel (nullptr); }

void NoniusEditor::resized()
{
    headerArea = { 0, 0, kWidth, 48 };
    spectrumArea = { 16, 60, kWidth - 32, 292 };
    findingsArea = { 16, 364, kWidth - 32 - 320 - 12, kHeight - 364 - 16 };
    metersArea = { findingsArea.getRight() + 12, 364, 320, kHeight - 364 - 16 };

    roleBox.setBounds (272, 11, 166, 26);
    for (int i = 0; i < 5; ++i) stageRects[i] = { 446 + i * 66, 8, 64, 32 };
    centralButton.setBounds (780, 11, 84, 26);
    holdButton.setBounds (870, 11, 78, 26);
    measureButton.setBounds (954, 11, 70, 26);
}

bool NoniusEditor::targetMode() const
{
    return shown != nullptr && ! proc.analyser().profileFor (shown->role).octaves.empty();
}

void NoniusEditor::timerCallback()
{
    // Rolle kann sich ohne Klick ändern (aus dem Spurnamen geraten, Projekt geladen)
    const int roleId = (int) proc.getRole() + 1;
    if (roleBox.getSelectedId() != roleId) roleBox.setSelectedId (roleId, juce::dontSendNotification);

    const auto name = proc.getTrackName();
    const bool playing = proc.isPlaying();
    bool full = name != shownTrackName || playing != wasPlaying;
    shownTrackName = name;
    wasPlaying = playing;

    // Messen-Knopf zeigt den Zustand: gefüllt "● Misst" bei Wiedergabe, umrandet "Pausiert" sonst
    const bool filled = measureButton.getProperties()["filled"];
    if (filled != playing || measureButton.getButtonText().isEmpty())
    {
        measureButton.getProperties().set ("filled", playing);
        measureButton.setButtonText (playing ? u8 ("● Misst") : u8 ("Pausiert"));
        measureButton.setColour (juce::TextButton::textColourOffId, playing ? C::bg : C::muted);
        measureButton.repaint();
    }

    if (! frozen)
    {
        auto s = proc.analyser().latest();
        if (s != shown) { shown = std::move (s); rebuildLists(); full = true; }
        live = proc.analyser().liveOctaves();
    }

    if (full) repaint();
    else repaint (spectrumArea);
}

void NoniusEditor::rebuildLists()
{
    issues.clear(); hints.clear();
    if (shown == nullptr) return;
    const int stage = proc.getStage();
    for (const auto& f : shown->findings)
    {
        if (nonius::stageOf (f.id) > stage) continue;
        // Rohspur: Vorschläge mit Fix (z. B. EQ-Schritte zum Ziel) sind der Fahrplan und gehören in die Liste
        const bool roadmap = shown->role == nonius::PluginRole::rawVocal && f.severity == nonius::Severity::info && ! f.fix.empty();
        if (isIssue (f) || roadmap) issues.push_back (&f);
        else if (f.severity != nonius::Severity::ok) hints.push_back (&f);
    }
}

void NoniusEditor::mouseUp (const juce::MouseEvent& e)
{
    const auto p = e.getPosition();
    if (showGlossary) { showGlossary = false; repaint(); return; } // Klick irgendwo schließt die Übersicht
    if (logoRect.contains (p)) { showInfoMenu(); return; }
    for (int i = 0; i < 5; ++i)
        if (stageRects[i].contains (p))
        {
            proc.setStage (i + 1);
            showAll = false;
            rebuildLists();
            repaint();
            return;
        }
    if (moreRect.contains (p))
    {
        showAll = ! showAll;
        repaint (findingsArea);
    }
}

void NoniusEditor::showInfoMenu()
{
   #ifndef NONIUS_VERSION_STRING
    #define NONIUS_VERSION_STRING "?"
   #endif
   #ifndef NONIUS_GIT_COMMIT
    #define NONIUS_GIT_COMMIT "?"
   #endif
    const auto& mixProf = proc.analyser().profile();
    const auto& voc = proc.analyser().vocalProfile();

    juce::PopupMenu m;
    m.addSectionHeader (u8 ("NONIUS ") + NONIUS_VERSION_STRING);
    m.addItem (1, u8 ("Build ") + NONIUS_GIT_COMMIT + u8 ("  ·  ") + juce::String (__DATE__), false);
    m.addSeparator();
    m.addItem (2, u8 ("Mix-Profil: ") + juce::String (mixProf.songs) + " Referenz-Songs", false);
    m.addItem (3, voc.empty() ? u8 ("Vocal-Profil: keins geladen")
                              : u8 ("Vocal-Profil: ") + juce::String (voc.songs) + " Referenz-Vocals", false);
    m.addSeparator();
    m.addItem (5, u8 ("Messgrößen erklärt"));
    m.addItem (4, u8 ("Ordner für eigene Profile öffnen (Dokumente/Nonius)"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (logoRect)),
                     [safe = juce::Component::SafePointer<NoniusEditor> (this)] (int result)
                     {
                         if (result == 5 && safe != nullptr) { safe->showGlossary = true; safe->repaint(); return; }
                         if (result != 4) return;
                         auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Nonius");
                         dir.createDirectory();
                         dir.startAsProcess();
                     });
}

void NoniusEditor::paintGlossary (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (C::panel);
    g.fillRoundedRectangle (area.toFloat(), 6.0f);
    g.setColour (C::range);
    g.drawRoundedRectangle (area.toFloat().reduced (0.5f), 6.0f, 1.0f);

    auto body = area.reduced (20, 14);
    auto title = body.removeFromTop (22);
    g.setColour (C::dim);
    g.setFont (caps (11.0f));
    g.drawText (u8 ("MESSGRÖSSEN ERKLÄRT"), title, juce::Justification::centredLeft);
    g.setFont (sans (12.0f));
    g.drawText (u8 ("Klick irgendwo schließt"), title, juce::Justification::centredRight);
    body.removeFromTop (6);

    const int n = (int) std::size (kGlossary), perCol = (n + 1) / 2;
    const int colW = (body.getWidth() - 24) / 2, rowH = body.getHeight() / perCol;
    for (int i = 0; i < n; ++i)
    {
        const auto& e = kGlossary[i];
        const juce::Rectangle<int> r (body.getX() + (i / perCol) * (colW + 24), body.getY() + (i % perCol) * rowH, colW, rowH);
        g.setColour (C::text);
        g.setFont (sans (13.0f, true));
        g.drawText (u8 (e.key), r.getX(), r.getY(), 160, 16, juce::Justification::topLeft, true);
        g.setColour (C::muted);
        g.setFont (sans (12.0f));
        g.drawFittedText (u8 (e.meaning), r.getX() + 162, r.getY(), colW - 162, 16, juce::Justification::topLeft, 1, 0.9f);
        g.setColour (C::accent);
        g.setFont (sans (12.0f));
        g.drawFittedText (u8 ("→ ") + u8 (e.tools), r.getX() + 162, r.getY() + 16, colW - 162, 16, juce::Justification::topLeft, 1, 0.9f);
    }
}

juce::String NoniusEditor::getTooltip()
{
    const auto p = getMouseXYRelative();
    if (logoRect.contains (p)) return u8 ("Version, Profile, Messgrößen erklärt");
    for (const auto& [r, label] : meterRows)
        if (r.contains (p))
            if (const auto* e = glossaryFor (label))
                return u8 (e->meaning) + "\n" + u8 ("→ ") + u8 (e->tools);
    return {};
}

void NoniusEditor::mouseMove (const juce::MouseEvent& e)
{
    bool clickable = moreRect.contains (e.getPosition()) || logoRect.contains (e.getPosition());
    for (auto& r : stageRects) clickable = clickable || r.contains (e.getPosition());
    setMouseCursor (clickable ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

// ====================================================================== Zeichnen
void NoniusEditor::paint (juce::Graphics& g)
{
    // Der Timer zeichnet 25-mal pro Sekunde nur das Klangbild neu: die anderen Bereiche
    // (Textlayout, Meter) nur, wenn sie wirklich im Neuzeichnen-Bereich liegen.
    if (g.clipRegionIntersects (headerArea))   { g.setColour (C::bg); g.fillRect (headerArea); paintHeader (g); }
    if (g.clipRegionIntersects (spectrumArea)) paintSpectrum (g, spectrumArea);
    if (g.clipRegionIntersects (findingsArea)) paintFindings (g, findingsArea);
    if (g.clipRegionIntersects (metersArea))   paintMeters (g, metersArea);
    if (g.clipRegionIntersects (getLocalBounds().withTrimmedTop (headerArea.getBottom())))
    {
        // Ränder zwischen den Feldern
        juce::RectangleList<int> gaps (getLocalBounds().withTrimmedTop (headerArea.getBottom()));
        gaps.subtract (spectrumArea); gaps.subtract (findingsArea); gaps.subtract (metersArea);
        g.setColour (C::bg);
        g.fillRectList (gaps);
    }
    if (showGlossary) paintGlossary (g, getLocalBounds().withTrimmedTop (headerArea.getBottom()).reduced (16, 12));

}

void NoniusEditor::paintHeader (juce::Graphics& g)
{
    g.setColour (C::panel);
    g.fillRect (headerArea);
    g.setColour (C::grid);
    g.drawHorizontalLine (headerArea.getBottom() - 1, 0.0f, (float) kWidth);

    // Logo: Nonius-Skala (zwei versetzte Teilungen)
    {
        const float x = 18.0f, y = 14.0f;
        g.setColour (C::accent);
        for (int i = 0; i < 5; ++i) g.fillRect (x + i * 4.0f, y, 1.5f, i == 0 || i == 4 ? 9.0f : 6.0f);
        g.setColour (C::text);
        for (int i = 0; i < 5; ++i) g.fillRect (x + 1.0f + i * 3.6f, y + 11.0f, 1.5f, i == 0 || i == 4 ? 9.0f : 6.0f);
    }
    g.setColour (C::text);
    g.setFont (caps (14.0f).withExtraKerningFactor (0.32f));
    g.drawText ("NONIUS", 46, 0, 100, 48, juce::Justification::centredLeft);

    // Spur
    const auto col = proc.getTrackColour();
    int x = 150;
    if (! col.isTransparent())
    {
        g.setColour (col);
        g.fillRoundedRectangle (150.0f, 18.0f, 10.0f, 12.0f, 2.0f);
        x = 166;
    }
    g.setColour (shownTrackName.isEmpty() ? C::dim : C::text);
    g.setFont (sans (14.0f, true));
    g.drawFittedText (shownTrackName.isEmpty() ? u8 ("Spur") : shownTrackName, x, 0, 264 - x, 48, juce::Justification::centredLeft, 1, 0.8f);

    // Stufen 1–5 mit Ampel-Balken (schlimmster Befund der Stufe)
    const int selected = proc.getStage();
    for (int i = 0; i < 5; ++i)
    {
        const auto r = stageRects[i];
        juce::Colour bar = C::grid2;
        if (shown != nullptr && shown->valid)
        {
            int worst = 99;
            for (const auto& f : shown->findings)
            {
                if (nonius::stageOf (f.id) != i + 1) continue;
                const int rank = f.severity == nonius::Severity::problem ? 0 : f.severity == nonius::Severity::warning ? 1
                               : f.severity == nonius::Severity::ok || f.severity == nonius::Severity::info ? 2 : 3;
                worst = std::min (worst, rank);
            }
            bar = worst == 0 ? C::bad : worst == 1 ? C::accent : worst == 2 ? C::ok : C::grid2;
        }
        const bool active = i + 1 <= selected;
        if (i + 1 == selected) { g.setColour (C::grid); g.fillRoundedRectangle (r.toFloat(), 3.0f); }
        g.setColour (active ? bar : bar.withAlpha (0.35f));
        g.fillRect (r.getX() + 6, r.getBottom() - 7, r.getWidth() - 12, 3);
        g.setColour (i + 1 == selected ? C::text : active ? C::muted : C::dim);
        g.setFont (sans (11.5f, i + 1 == selected));
        g.drawText (juce::String (i + 1) + " " + u8 (kStageNames[i]), r.withTrimmedBottom (8), juce::Justification::centred);
    }
}

void NoniusEditor::paintSpectrum (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (C::bg);
    g.fillRect (area);
    g.setColour (C::panel);
    g.fillRoundedRectangle (area.toFloat(), 6.0f);

    const auto& prof = proc.analyser().profileFor (shown != nullptr ? shown->role : proc.getRole());
    const bool target = targetMode();
    const bool vocal = shown != nullptr && nonius::isVocal (shown->role);
    const bool valid = shown != nullptr && shown->valid;
    const bool beat = shown != nullptr && shown->role == nonius::PluginRole::beat;

    // Titelzeile
    g.setColour (C::dim);
    g.setFont (caps (11.0f));
    g.drawText ("KLANGBILD", area.getX() + 16, area.getY() + 10, 200, 16, juce::Justification::centredLeft);

    juce::String status;
    if (valid) status << juce::String ((int) std::round (shown->secondsMeasured)) << " s gemessen";
    else       status << juce::String (proc.analyser().bufferedSeconds(), 1) << " s gesammelt";
    if (target) status << u8 ("  ·  gegen ") << prof.songs << (vocal ? " Referenz-Vocals" : " Referenz-Songs");
    else if (valid) status << u8 ("  ·  Vocal-Zielkurve folgt (braucht Referenz-Stems)");
    if (! proc.isPlaying()) status << u8 ("  ·  pausiert");
    g.setColour (frozen ? C::accent : C::muted);
    g.setFont (sans (12.0f));
    g.drawText (frozen ? u8 ("FESTGEHALTEN  ·  ") + status : status, area.getX() + 120, area.getY() + 10, area.getWidth() - 136, 16,
                juce::Justification::centredRight);

    // Diagrammfläche
    const juce::Rectangle<float> chart ((float) area.getX() + 48.0f, (float) area.getY() + 40.0f, (float) area.getWidth() - 64.0f, 212.0f);
    const auto& centres = valid ? shown->octaveCentres : prof.octaveCentres;
    if (centres.empty()) return;
    const double fMin = centres.front() / std::sqrt (2.0), fMax = centres.back() * std::sqrt (2.0);
    auto xOf = [&] (double f) { return chart.getX() + (float) (std::log (f / fMin) / std::log (fMax / fMin)) * chart.getWidth(); };

    const double yTop = target ? 12.0 : 0.0, yBottom = target ? -12.0 : -42.0, yStep = target ? 3.0 : 6.0;
    auto yOf = [&] (double db)
    {
        db = juce::jlimit (yBottom, yTop, db);
        return chart.getY() + (float) ((yTop - db) / (yTop - yBottom)) * chart.getHeight();
    };

    g.setFont (mono (11.0f));
    for (double db = yTop; db >= yBottom - 0.01; db -= yStep)
    {
        const float y = yOf (db);
        g.setColour (target && std::fabs (db) < 0.01 ? C::range : C::grid);
        g.drawHorizontalLine ((int) y, chart.getX(), chart.getRight());
        g.setColour (C::dim);
        const auto label = (db > 0 ? "+" : "") + juce::String ((int) db);
        g.drawText (db < 0 ? u8 ("−") + juce::String ((int) -db) : label, (int) area.getX() + 4, (int) y - 7, 38, 14, juce::Justification::centredRight);
    }
    for (double f : { 50.0, 100.0, 250.0, 1000.0, 2500.0, 5000.0, 12000.0 })
    {
        if (f < fMin || f > fMax) continue;
        const float x = xOf (f);
        g.setColour (C::grid);
        g.drawVerticalLine ((int) x, chart.getY(), chart.getBottom());
        g.setColour (C::dim);
        g.drawText (hzText (f), (int) x - 30, (int) chart.getBottom() + 4, 60, 14, juce::Justification::centred);
    }

    // Bänder
    for (std::size_t i = 0; i < centres.size(); ++i)
    {
        const double c = centres[i];
        const float x0 = xOf (c / std::sqrt (2.0)) + 4.0f, x1 = xOf (c * std::sqrt (2.0)) - 4.0f;
        const bool hasRange = target && i < prof.octaves.size() && prof.octaves[i].valid;
        const bool notRated = beat && c >= 250.0 && c <= 4000.0;
        const double p50 = hasRange ? prof.octaves[i].p50 : 0.0;

        if (hasRange)
        {
            const auto& r = prof.octaves[i];
            const float ya = yOf (r.p90 - p50), yb = yOf (r.p10 - p50);
            g.setColour (notRated ? C::slot.withAlpha (0.4f) : C::slot);
            g.fillRect (juce::Rectangle<float> (x0 - 2.0f, ya, x1 - x0 + 4.0f, std::max (2.0f, yb - ya)));
        }

        if (valid && i < shown->octaveBalance.size() && (target ? hasRange : true))
        {
            const double v = shown->octaveBalance[i];
            const double shownV = target ? v - p50 : v;
            juce::Colour col = C::text.withAlpha (0.75f);
            if (hasRange)
            {
                const auto& r = prof.octaves[i];
                const double out = v > r.p90 ? v - r.p90 : v < r.p10 ? r.p10 - v : 0.0;
                const double fromMid = std::fabs (v - p50);
                if (out > 1.5) col = fromMid > 4.0 ? C::bad : C::accent;
                if (notRated) col = C::text.withAlpha (0.3f);
            }
            const float y0 = target ? yOf (0.0) : chart.getBottom(), y1 = yOf (shownV);
            g.setColour (col);
            g.fillRect (juce::Rectangle<float> (x0 + 6.0f, std::min (y0, y1), std::max (2.0f, x1 - x0 - 12.0f), std::max (2.0f, std::fabs (y1 - y0))));
            if (target && std::fabs (shownV) > yTop)
            {
                // außerhalb der Skala: Wert dazuschreiben statt nur am Rand kleben
                g.setColour (C::bg);
                g.setFont (sans (12.0f, true));
                const float ty = shownV > 0 ? y1 + 4.0f : y1 - 20.0f;
                g.drawText ((shownV > 0 ? "+" : u8 ("−")) + juce::String (std::fabs (shownV), 1) + " dB",
                            juce::Rectangle<float> (x0, ty, x1 - x0, 16.0f), juce::Justification::centred);
            }
        }

        // Live-Strich: was gerade spielt
        if (i < live.size() && live.size() == centres.size() && (! target || hasRange))
        {
            const float y = yOf (target ? live[i] - p50 : live[i]);
            g.setColour (C::text.withAlpha (notRated ? 0.3f : 0.9f));
            g.fillRect (juce::Rectangle<float> (x0, y - 1.0f, x1 - x0, 2.0f));
        }
    }

    // Nummerierte Marker für Befunde mit Frequenz
    for (std::size_t n = 0; n < issues.size(); ++n)
    {
        const auto* f = issues[n];
        if (f->frequency <= 0 || f->frequency < fMin || f->frequency > fMax) continue;
        const float x = xOf (f->frequency);
        const auto col = severityColour (f->severity);
        g.setColour (col.withAlpha (0.5f));
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine ({ x, chart.getY() + 20.0f, x, chart.getBottom() }, dashes, 2, 1.0f);
        const juce::Rectangle<float> box (x - 9.0f, chart.getY() - 2.0f, 18.0f, 18.0f);
        g.setColour (col);
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (C::bg);
        g.setFont (sans (11.5f, true));
        g.drawText (juce::String ((int) n + 1), box, juce::Justification::centred);
    }

    // Legende / Hinweis
    g.setFont (sans (11.0f));
    g.setColour (C::dim);
    const int ly = (int) chart.getBottom() + 22;
    if (target)
    {
        g.setColour (C::slot);
        g.fillRect (area.getX() + 16, ly + 3, 14, 8);
        g.setColour (C::dim);
        juce::String legend = u8 ("Ziel P10–P90, 0 = Median  ·  Balken: gemessen  ·  Strich: jetzt");
        if (beat) legend << u8 ("  ·  250 Hz–4 kHz beim Beat nicht bewertet (dort sitzt das Vocal)");
        g.drawText (legend, area.getX() + 36, ly, 900, 14, juce::Justification::centredLeft);
    }

    if (! valid)
    {
        g.setColour (C::muted);
        g.setFont (sans (15.0f));
        const auto msg = proc.isPlaying() ? u8 ("Sammle Material … erste Auswertung nach 5 s")
                                          : u8 ("Wiedergabe starten: NONIUS misst, solange der Song läuft");
        g.drawText (msg, chart.toNearestInt(), juce::Justification::centred);
    }
}

void NoniusEditor::paintFindings (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (C::bg);
    g.fillRect (area);
    g.setColour (C::panel);
    g.fillRoundedRectangle (area.toFloat(), 6.0f);
    moreRect = {};

    g.setColour (C::dim);
    g.setFont (caps (11.0f));
    g.drawText ("BEFUND", area.getX() + 16, area.getY() + 10, 200, 16, juce::Justification::centredLeft);
    g.setFont (sans (12.0f));
    const int stage = proc.getStage();
    g.drawText (stage == 1 ? u8 ("Stufe 1") : u8 ("Stufe 1–") + juce::String (stage), area.getRight() - 216, area.getY() + 10, 200, 16,
                juce::Justification::centredRight);

    auto body = area.reduced (16, 0).withTrimmedTop (36).withTrimmedBottom (10);
    if (shown == nullptr || ! shown->valid)
    {
        g.setColour (C::dim);
        g.setFont (sans (14.0f));
        g.drawText (u8 ("Noch keine Auswertung"), body, juce::Justification::topLeft);
        return;
    }

    if (showAll)
    {
        // Kompakte Liste aller Punkte
        std::vector<const nonius::Finding*> all (issues);
        all.insert (all.end(), hints.begin(), hints.end());
        for (int i = 0; i < (int) all.size(); ++i)
        {
            const auto* f = all[(size_t) i];
            juce::String line = u8 (f->text);
            if (! f->fix.empty()) line << u8 ("  →  ") << u8 (f->fix);
            const bool twoLines = juce::GlyphArrangement::getStringWidth (sans (12.5f), line) > (float) body.getWidth() - 174.0f;
            const int rowH = twoLines ? 36 : 20;
            if (rowH > body.getHeight() - 22) break;
            auto row = body.removeFromTop (rowH);
            g.setColour (severityColour (f->severity));
            g.fillEllipse ((float) row.getX(), (float) row.getY() + 6.5f, 7.0f, 7.0f);
            g.setColour (C::text);
            g.setFont (sans (13.0f, true));
            g.drawText (u8 (f->title), row.getX() + 16, row.getY(), 150, 20, juce::Justification::centredLeft, true);
            g.setColour (C::muted);
            g.setFont (sans (12.5f));
            g.drawFittedText (line, row.getX() + 170, row.getY() + 2, row.getWidth() - 170, rowH - 2, juce::Justification::topLeft, twoLines ? 2 : 1, 1.0f);
        }
        moreRect = body.removeFromBottom (20);
        g.setColour (C::accent);
        g.setFont (sans (12.5f, true));
        g.drawText (u8 ("← zurück zu den wichtigsten Punkten"), moreRect, juce::Justification::centredLeft);
        return;
    }

    if (issues.empty())
    {
        g.setColour (C::ok);
        g.setFont (sans (15.0f, true));
        g.drawText (u8 ("Nichts Auffälliges in Stufe 1–") + juce::String (stage), body.removeFromTop (24), juce::Justification::centredLeft);
        g.setColour (C::muted);
        g.setFont (sans (13.0f));
        g.drawText (u8 ("Alle Messwerte liegen im Zielbereich."), body.removeFromTop (20), juce::Justification::centredLeft);
    }

    // Höhe pro Punkt richtet sich nach der Textlänge (bis 2 Zeilen je Text und Fix), nichts wird abgeschnitten
    const int tx0 = body.getX() + 36, tw = body.getWidth() - 36;
    auto linesFor = [tw] (const juce::Font& font, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (font, text) > (float) tw - 4.0f ? 2 : 1;
    };
    int shownItems = 0;
    for (std::size_t n = 0; n < issues.size() && shownItems < 4; ++n)
    {
        const auto* f = issues[n];
        juce::String line = u8 (f->text);
        if (! f->measured.empty() && ! line.contains (u8 (f->measured))) line << u8 ("  (") << u8 (f->measured) << ")";
        const juce::String fix = f->fix.empty() ? juce::String() : u8 ("→ ") + u8 (f->fix);
        const int textLines = linesFor (sans (13.0f), line);
        const int fixLines = fix.isEmpty() ? 0 : linesFor (mono (13.0f), fix);
        const int itemH = 22 + 17 * textLines + 17 * fixLines + 8;
        if (itemH > body.getHeight() - 22) break;
        ++shownItems;

        auto item = body.removeFromTop (itemH);
        const auto col = severityColour (f->severity);

        const juce::Rectangle<float> badge ((float) item.getX(), (float) item.getY() + 1.0f, 22.0f, 22.0f);
        g.setColour (col);
        g.fillEllipse (badge);
        g.setColour (C::bg);
        g.setFont (sans (12.0f, true));
        g.drawText (juce::String ((int) n + 1), badge, juce::Justification::centred);

        const int tx = tx0;
        g.setColour (C::text);
        g.setFont (sans (15.0f, true));
        const auto title = u8 (f->title);
        g.drawText (title, tx, item.getY(), tw, 22, juce::Justification::centredLeft, true);
        const int titleW = (int) juce::GlyphArrangement::getStringWidth (sans (15.0f, true), title);

        g.setFont (sans (11.0f));
        g.setColour (C::dim);
        const int st = nonius::stageOf (f->id);
        juce::String tag;
        if (st >= 1) tag << "Stufe " << st << " " << u8 (kStageNames[st - 1]) << u8 ("  ·  ");
        tag << f->id << u8 ("  ·  ") << nonius::severityLabel (f->severity);
        if (! f->note.empty()) tag << u8 ("  ·  ") << u8 (f->note);
        g.drawText (tag, tx + titleW + 12, item.getY() + 1, tw - titleW - 12, 22, juce::Justification::centredLeft, true);

        int y = item.getY() + 22;
        g.setColour (C::muted);
        g.setFont (sans (13.0f));
        g.drawFittedText (line, tx, y, tw, 17 * textLines, juce::Justification::topLeft, textLines, 1.0f);
        y += 17 * textLines;

        if (fixLines > 0)
        {
            g.setColour (C::accent);
            g.setFont (mono (13.0f));
            g.drawFittedText (fix, tx, y, tw, 17 * fixLines, juce::Justification::topLeft, fixLines, 1.0f);
        }
    }

    const int moreIssues = (int) issues.size() - shownItems;
    if (moreIssues > 0 || ! hints.empty())
    {
        juce::String more;
        if (moreIssues > 0) more << "+ " << moreIssues << (moreIssues == 1 ? " weiterer Punkt" : " weitere Punkte");
        if (! hints.empty())
        {
            if (more.isNotEmpty()) more << u8 ("  ·  ");
            more << "+ " << (int) hints.size() << (hints.size() == 1 ? " kleinerer Hinweis" : " kleinere Hinweise");
        }
        moreRect = { area.getX() + 16 + 36, area.getBottom() - 30, 400, 20 };
        g.setColour (C::muted);
        g.setFont (sans (12.5f));
        g.drawText (more + u8 ("  (anzeigen)"), moreRect, juce::Justification::centredLeft);
    }
}

std::vector<NoniusEditor::Meter> NoniusEditor::buildMeters() const
{
    std::vector<Meter> m;
    if (shown == nullptr) return m;
    const auto& t = shown->track;
    const auto& prof = proc.analyser().profile();
    const bool valid = shown->valid;

    auto fixed = [&] (const char* label, bool has, double v, double lo, double hi, juce::String text)
    {
        Meter x;
        x.label = u8 (label); x.hasValue = valid && has; x.value = v; x.lo = lo; x.hi = hi;
        x.valueText = x.hasValue ? text : u8 ("–");
        m.push_back (x);
    };
    auto fromProfile = [&] (const char* label, const char* key, bool has, double v, juce::String text)
    {
        const auto it = prof.ranges.find (key);
        if (it == prof.ranges.end() || ! it->second.valid) return false;
        Meter x;
        x.label = u8 (label); x.hasValue = valid && has; x.value = v;
        x.lo = it->second.p10; x.hi = it->second.p90; x.mid = it->second.p50; x.hasMid = true;
        x.valueText = x.hasValue ? text : u8 ("–");
        m.push_back (x);
        return true;
    };
    const auto& vprof = proc.analyser().vocalProfile();
    const bool vocalTarget = ! vprof.empty();
    // Vocal-Wert gegen die Referenz-Vocals, ohne Profil gegen feste Startwerte (lo/hi)
    auto vocalMeter = [&] (const char* label, const char* key, bool has, double v, double lo, double hi, juce::String text)
    {
        Meter x;
        x.label = u8 (label); x.hasValue = valid && has; x.value = v; x.lo = lo; x.hi = hi;
        if (vocalTarget)
            if (const auto it = vprof.ranges.find (key); it != vprof.ranges.end() && it->second.valid)
            {
                x.lo = it->second.p10; x.hi = it->second.p90; x.mid = it->second.p50; x.hasMid = true;
            }
        if (! x.hasMid && lo >= hi) { x.hasTarget = false; x.lo = v - 12.0; x.hi = v + 12.0; } // nur Wert zeigen
        x.valueText = x.hasValue ? text : u8 ("–");
        m.push_back (x);
    };
    auto num = [] (double v, int d, const char* unit)
    {
        const auto s = d == 0 ? juce::String (juce::roundToInt (std::fabs (v))) : juce::String (std::fabs (v), d);
        return (v < 0 && s.containsAnyOf ("123456789") ? u8 ("−") : juce::String()) + s + " " + u8 (unit);
    };

    switch (shown->role)
    {
        case nonius::PluginRole::rawVocal:
            fixed ("Peaks beim Singen/Rappen", t.activePeakDb > -149.0, t.activePeakDb, -12.0, -6.0, num (t.activePeakDb, 1, "dBFS"));
            fixed ("Clipping", true, t.clippingRuns, -0.25, 0.25, juce::String (t.clippingRuns) + " Stellen");
            fixed ("Popps", true, t.plosives.perMinute, 0.0, 2.0, num (t.plosives.perMinute, 1, "/min"));
            fixed ("Rauschen in Pausen (A)", t.noiseFloorReliable && t.noiseFloorDbA > -149.0, t.noiseFloorDbA, -100.0, -60.0, num (t.noiseFloorDbA, 0, "dBFS"));
            fixed ("Raumhall (Abklingwert)", t.reverbDecays >= 10, t.reverbSeconds, 0.0, 0.32, num (t.reverbSeconds, 2, "s"));
            fixed ("Rumpeln unter 80 Hz", t.rumbleDb > -149.0, t.rumbleDb, -60.0, -25.0, num (t.rumbleDb, 0, "dB"));
            vocalMeter ("Silben-Dynamik (Ziel: fertige Vocals)", "Silben-Dynamik", shown->vocalDyn.syllableDb > 0, shown->vocalDyn.syllableDb, 0, 0, num (shown->vocalDyn.syllableDb, 1, "dB"));
            break;

        case nonius::PluginRole::leadVocal:
        {
            const auto& d = shown->vocalDyn;
            vocalMeter ("Silben-Dynamik (Kompression)", "Silben-Dynamik", d.syllableDb > 0, d.syllableDb, 0, 0, num (d.syllableDb, 1, "dB"));
            vocalMeter ("Zischlaute (S zu Vokal)", "Zischlaute (S zu Vokal)", t.sibilantToVowelDb > -149.0, t.sibilantToVowelDb, -14.0, -1.0, num (t.sibilantToVowelDb, 1, "dB"));
            vocalMeter ("Crest 100 ms", "Crest 100 ms", true, t.crest100msDb, 6.5, 14.0, num (t.crest100msDb, 1, "dB"));
            vocalMeter ("Spektrale Neigung", "Spektrale Neigung", true, t.spectrum.tiltDbPerOct, 0, 0, num (t.spectrum.tiltDbPerOct, 1, "dB/Okt."));
            vocalMeter ("Abstand Wort zu Lücke", "Abstand Wort zu Lücke", d.gapDepthDb > 0, d.gapDepthDb, 0, 0, num (d.gapDepthDb, 1, "dB"));
            vocalMeter ("Breite (Seite zu Mitte)", "Breite (Seite zu Mitte)", shown->sideToMid > -100.0, shown->sideToMid, 0, 0, num (shown->sideToMid, 1, "dB"));
            fixed ("Rumpeln unter 80 Hz", t.rumbleDb > -149.0, t.rumbleDb, -60.0, -25.0, num (t.rumbleDb, 0, "dB"));
        }
            break;

        case nonius::PluginRole::beat:
            fixed ("True Peak", true, t.truePeakDb, -6.0, -1.0, num (t.truePeakDb, 1, "dBTP"));
            fromProfile ("Bass unter 250 Hz", "Bass < 250 Hz", true, shown->four.low, num (shown->four.low, 1, "dB"));
            fromProfile (u8"Höhen über 8 kHz", u8"Höhen > 8 kHz", true, shown->four.high, num (shown->four.high, 1, "dB"));
            fromProfile ("Spektrale Neigung", "Spektrale Neigung", true, t.spectrum.tiltDbPerOct, num (t.spectrum.tiltDbPerOct, 1, "dB/Okt."));
            fromProfile ("Breite (Seite zu Mitte)", "Breite (Seite zu Mitte)", shown->sideToMid > -100.0, shown->sideToMid, num (shown->sideToMid, 1, "dB"));
            if (fromProfile ("Bass-Seite unter 120 Hz", "Bass-Seite < 120 Hz", t.sideBelow120Db > -149.0, t.sideBelow120Db, num (t.sideBelow120Db, 1, "dB")))
                m.back().lowerIsFine = true; // monoer als die Referenzen ist kein Fehler
            break;

        case nonius::PluginRole::sum:
            fromProfile ("Lautheit integriert", "Lautheit integriert", t.loudness.integratedLufs > -149.0, t.loudness.integratedLufs, num (t.loudness.integratedLufs, 1, "LUFS"));
            fixed ("True Peak", true, t.truePeakDb, -3.0, -1.0, num (t.truePeakDb, 1, "dBTP"));
            fromProfile ("Crest 100 ms", "Crest 100 ms", true, t.crest100msDb, num (t.crest100msDb, 1, "dB"));
            fromProfile ("Crest 1 s", "Crest 1 s", true, t.crest1sDb, num (t.crest1sDb, 1, "dB"));
            fromProfile ("Spektrale Neigung", "Spektrale Neigung", true, t.spectrum.tiltDbPerOct, num (t.spectrum.tiltDbPerOct, 1, "dB/Okt."));
            fromProfile ("Breite (Seite zu Mitte)", "Breite (Seite zu Mitte)", shown->sideToMid > -100.0, shown->sideToMid, num (shown->sideToMid, 1, "dB"));
            if (fromProfile ("Bass-Seite unter 120 Hz", "Bass-Seite < 120 Hz", t.sideBelow120Db > -149.0, t.sideBelow120Db, num (t.sideBelow120Db, 1, "dB")))
                m.back().lowerIsFine = true; // monoer als die Referenzen ist kein Fehler
            break;
    }
    return m;
}

void NoniusEditor::paintMeters (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (C::bg);
    g.fillRect (area);
    g.setColour (C::panel);
    g.fillRoundedRectangle (area.toFloat(), 6.0f);

    g.setColour (C::dim);
    g.setFont (caps (11.0f));
    g.drawText ("WERTE GEGEN ZIEL", area.getX() + 16, area.getY() + 10, 250, 16, juce::Justification::centredLeft);

    auto body = area.reduced (16, 0).withTrimmedTop (38);
    const auto meters = buildMeters();
    meterRows.clear();
    if (meters.empty())
    {
        g.setFont (sans (13.0f));
        g.drawText (u8 ("Noch keine Werte"), body, juce::Justification::topLeft);
        return;
    }

    for (const auto& mt : meters)
    {
        auto row = body.removeFromTop (33);
        meterRows.emplace_back (row, mt.label);
        g.setColour (C::muted);
        g.setFont (sans (12.5f));
        g.drawText (mt.hasTarget ? mt.label : mt.label + u8 ("  (Ziel folgt)"), row.getX(), row.getY(), row.getWidth() - 100, 16, juce::Justification::centredLeft, true);
        g.setColour (mt.hasValue ? C::text : C::dim);
        g.setFont (mono (13.0f));
        g.drawText (mt.valueText, row.getRight() - 110, row.getY(), 110, 16, juce::Justification::centredRight);

        // Skala: Zielbereich in der Mitte, links und rechts Platz für Ausreißer
        const double span = std::max (mt.hi - mt.lo, 0.5);
        double smin = mt.lo - 0.75 * span, smax = mt.hi + 0.75 * span;
        if (mt.hasValue) { smin = std::min (smin, mt.value - 0.1 * span); smax = std::max (smax, mt.value + 0.1 * span); }
        const juce::Rectangle<float> bar ((float) row.getX(), (float) row.getY() + 21.0f, (float) row.getWidth(), 4.0f);
        auto xOf = [&] (double v) { return bar.getX() + (float) ((v - smin) / (smax - smin)) * bar.getWidth(); };

        g.setColour (C::range);
        g.fillRoundedRectangle (bar, 2.0f);
        if (! mt.hasTarget)
        {
            // noch kein Ziel (braucht Referenz-Stems): nur Wert auf fester Skala zeigen
            if (mt.hasValue)
            {
                const float x = bar.getX() + (float) juce::jlimit (0.0, 1.0, (mt.value - mt.lo) / (mt.hi - mt.lo)) * bar.getWidth();
                g.setColour (C::muted);
                g.fillRoundedRectangle (x - 2.0f, bar.getY() - 4.0f, 4.0f, bar.getHeight() + 8.0f, 1.5f);
            }
            continue;
        }
        g.setColour (C::dim);
        g.fillRect (juce::Rectangle<float> (xOf (mt.lo), bar.getY(), xOf (mt.hi) - xOf (mt.lo), bar.getHeight()));
        if (mt.hasMid)
        {
            g.setColour (C::muted);
            g.fillRect (xOf (mt.mid) - 0.5f, bar.getY() - 2.0f, 1.0f, bar.getHeight() + 4.0f);
        }
        if (mt.hasValue)
        {
            const double out = mt.value > mt.hi ? mt.value - mt.hi : (mt.value < mt.lo && ! mt.lowerIsFine) ? mt.lo - mt.value : 0.0;
            const auto col = out <= 0 ? C::ok : out < 0.5 * span ? C::accent : C::bad;
            g.setColour (col);
            g.fillRoundedRectangle (xOf (mt.value) - 2.0f, bar.getY() - 4.0f, 4.0f, bar.getHeight() + 8.0f, 1.5f);
        }
    }
}
