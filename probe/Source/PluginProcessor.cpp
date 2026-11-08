#include "PluginProcessor.h"
#include "PluginEditor.h"

#if JUCE_WINDOWS
 #include <windows.h>
#else
 #include <unistd.h>
#endif

namespace
{
    // Wird von allen Instanzen geteilt, solange sie im selben Prozess laufen.
    struct ProcessWideRegistry
    {
        std::atomic<int> liveInstances { 0 };
        std::atomic<int> nextId { 1 };
    };

    juce::uint32 getProcessId()
    {
       #if JUCE_WINDOWS
        return (juce::uint32) GetCurrentProcessId();
       #else
        return (juce::uint32) getpid();
       #endif
    }

    double nowMs()
    {
        return juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks()) * 1000.0;
    }
}

//==============================================================================
class ProbeLog
{
public:
    ProbeLog (int id)
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("NoniusProbe");
        dir.createDirectory();

        auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
        file = dir.getChildFile ("probe-" + stamp + "-pid" + juce::String (getProcessId())
                                 + "-inst" + juce::String (id) + ".csv");
        stream = std::make_unique<juce::FileOutputStream> (file);

        if (stream->openedOk())
            stream->writeText ("wall_ms;event;details\n", false, false, nullptr);
    }

    void write (const juce::String& event, const juce::String& details)
    {
        if (stream == nullptr || ! stream->openedOk())
            return;

        auto line = juce::String (nowMs(), 3) + ";" + event + ";" + details.replace (";", ",") + "\n";
        stream->writeText (line, false, false, nullptr);
        stream->flush();
    }

    juce::File file;

private:
    std::unique_ptr<juce::FileOutputStream> stream;
};

//==============================================================================
// Frage 7: Shared Memory des Betriebssystems (nur Windows).
class SharedMemoryTest
{
public:
    static constexpr int numSlots = 64;

    explicit SharedMemoryTest (int id) : instanceId (id)
    {
       #if JUCE_WINDOWS
        const DWORD size = (DWORD) (sizeof (Slot) * numSlots);
        handle = CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, L"Local\\NoniusProbeSharedMemory");
        lastError = (int) GetLastError();

        if (handle != nullptr)
        {
            alreadyExisted = (lastError == ERROR_ALREADY_EXISTS);
            view = MapViewOfFile (handle, FILE_MAP_ALL_ACCESS, 0, 0, size);

            if (view == nullptr)
                lastError = (int) GetLastError();
        }
       #endif
    }

    ~SharedMemoryTest()
    {
       #if JUCE_WINDOWS
        if (auto* slots = getSlots())
            for (int i = 0; i < numSlots; ++i)
                if (slots[i].pid == getProcessId() && slots[i].instanceId == instanceId)
                    slots[i] = {};

        if (view != nullptr)   UnmapViewOfFile (view);
        if (handle != nullptr) CloseHandle (handle);
       #endif
    }

    juce::String describeOpen() const
    {
       #if JUCE_WINDOWS
        if (view == nullptr)
            return "FEHLER winerror=" + juce::String (lastError);

        return alreadyExisted ? "geoeffnet (existierte schon -> andere Instanz/Prozess hat es angelegt)"
                              : "neu angelegt (erste Instanz)";
       #else
        return "nicht verfuegbar (nur Windows)";
       #endif
    }

    // Eigenen Herzschlag schreiben und alle anderen Einträge zurückgeben.
    juce::String heartbeatAndList (juce::int64 counter)
    {
        auto* slots = getSlots();

        if (slots == nullptr)
            return "kein Shared Memory";

        const auto pid = getProcessId();
        int mine = -1;

        for (int i = 0; i < numSlots && mine < 0; ++i)
            if (slots[i].pid == pid && slots[i].instanceId == instanceId)
                mine = i;

        for (int i = 0; i < numSlots && mine < 0; ++i)
            if (slots[i].pid == 0)
                mine = i;

        if (mine >= 0)
        {
            slots[mine].pid = pid;
            slots[mine].instanceId = instanceId;
            slots[mine].counter = counter;
        }

        juce::StringArray others;

        for (int i = 0; i < numSlots; ++i)
            if (i != mine && slots[i].pid != 0)
                others.add ("pid" + juce::String (slots[i].pid) + "/inst" + juce::String (slots[i].instanceId)
                            + "=" + juce::String (slots[i].counter));

        return "sehe " + juce::String (others.size()) + " andere: " + others.joinIntoString (" ");
    }

private:
    struct Slot
    {
        juce::uint32 pid = 0;
        juce::int32 instanceId = 0;
        juce::int64 counter = 0;
    };

    Slot* getSlots() const
    {
       #if JUCE_WINDOWS
        return static_cast<Slot*> (view);
       #else
        return nullptr;
       #endif
    }

    int instanceId = 0;
    int lastError = 0;
    bool alreadyExisted = false;
    void* handle = nullptr;
    void* view = nullptr;
};

//==============================================================================
NoniusProbeProcessor::NoniusProbeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    juce::SharedResourcePointer<ProcessWideRegistry> registry;
    instanceId = registry->nextId.fetch_add (1);
    const int live = registry->liveInstances.fetch_add (1) + 1;

    log = std::make_unique<ProbeLog> (instanceId);
    shm = std::make_unique<SharedMemoryTest> (instanceId);

    logEvent ("START", "pid=" + juce::String (getProcessId())
                       + " instanz=" + juce::String (instanceId)
                       + " instanzen_im_prozess=" + juce::String (live)
                       + " registry_adresse=" + juce::String::toHexString ((juce::pointer_sized_int) (void*) &(*registry))
                       + " host=" + juce::PluginHostType().getHostDescription()
                       + " juce=" + juce::SystemStats::getJUCEVersion());
    logEvent ("SHARED_MEMORY", shm->describeOpen());

    lastStatusTime = nowMs();
    startTimerHz (10);
}

NoniusProbeProcessor::~NoniusProbeProcessor()
{
    stopTimer();
    juce::SharedResourcePointer<ProcessWideRegistry> registry;
    const int live = registry->liveInstances.fetch_sub (1) - 1;
    logEvent ("ENDE", "instanzen_im_prozess_danach=" + juce::String (live));
}

void NoniusProbeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    logEvent ("PREPARE", "samplerate=" + juce::String (sampleRate)
                         + " blockgroesse=" + juce::String (samplesPerBlock)
                         + " latenz=" + juce::String (getLatencySamples()));
}

void NoniusProbeProcessor::releaseResources()
{
    logEvent ("RELEASE");
}

bool NoniusProbeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void NoniusProbeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Audio bleibt unverändert. Hier nur Zähler und Zeitstempel (echtzeitsicher, ohne Speicher/Datei).
    const auto ticks = juce::Time::getHighResolutionTicks();

    juce::int64 timeInSamples = -1;
    bool isPlaying = false;
    bool hasPosition = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            hasPosition = true;
            isPlaying = pos->getIsPlaying();

            if (auto t = pos->getTimeInSamples())
                timeInSamples = *t;
        }
    }

    float peak = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));

    float previous = peakSinceLastStatus.load();
    while (peak > previous && ! peakSinceLastStatus.compare_exchange_weak (previous, peak)) {}

    lastTimeInSamples.store (timeInSamples);
    lastBlockTicks.store (ticks);
    lastIsPlaying.store (isPlaying);
    lastHasPosition.store (hasPosition);
    lastNonRealtime.store (isNonRealtime());
    blockCounter.fetch_add (1);
}

void NoniusProbeProcessor::timerCallback()
{
    const bool playing = lastIsPlaying.load();
    const bool offline = lastNonRealtime.load();

    if (playing != lastLoggedPlaying)
    {
        logEvent (playing ? "TRANSPORT_PLAY" : "TRANSPORT_STOP");
        lastLoggedPlaying = playing;
    }

    if (offline != lastLoggedNonRealtime)
    {
        logEvent (offline ? "OFFLINE_RENDER_START" : "OFFLINE_RENDER_ENDE");
        lastLoggedNonRealtime = offline;
    }

    // Frage 4: Zeitposition + Zeitpunkt des letzten Blocks, 10x pro Sekunde.
    // Auswertung: Für denselben wall_ms-Moment die timeInSamples aller Instanzen vergleichen.
    const auto blockTicks = lastBlockTicks.load();
    const double blockMs = blockTicks > 0 ? juce::Time::highResolutionTicksToSeconds (blockTicks) * 1000.0 : 0.0;

    if (playing)
        logEvent ("POS", "time_in_samples=" + juce::String (lastTimeInSamples.load())
                         + " block_wall_ms=" + juce::String (blockMs, 3));

    // Fragen 2, 3, 6, 7: Status einmal pro Sekunde.
    const double now = nowMs();

    if (now - lastStatusTime >= 1000.0)
    {
        const auto blocks = blockCounter.load();
        const double seconds = (now - lastStatusTime) / 1000.0;
        const double blocksPerSecond = (double) (blocks - blocksAtLastStatus) / seconds;
        const float peak = peakSinceLastStatus.exchange (0.0f);

        juce::SharedResourcePointer<ProcessWideRegistry> registry;

        lastStatusLine = "bloecke_pro_s=" + juce::String (blocksPerSecond, 1)
                       + " spielt=" + juce::String (playing ? 1 : 0)
                       + " position_vorhanden=" + juce::String (lastHasPosition.load() ? 1 : 0)
                       + " peak_db=" + juce::String (juce::Decibels::gainToDecibels (peak, -120.0f), 1)
                       + " offline=" + juce::String (offline ? 1 : 0)
                       + " instanzen_im_prozess=" + juce::String (registry->liveInstances.load());

        logEvent ("STATUS", lastStatusLine);
        logEvent ("SHARED_MEMORY_HERZSCHLAG", shm->heartbeatAndList (blocks));

        blocksAtLastStatus = blocks;
        lastStatusTime = now;
    }
}

void NoniusProbeProcessor::updateTrackProperties (const TrackProperties& properties)
{
    // Frage 5: Name und Farbe kommen evtl. getrennt -> beides roh mitschreiben.
    juce::String details = "name=";
    details << (properties.name.has_value() ? "\"" + *properties.name + "\"" : juce::String ("(nicht gemeldet)"));
    details << " farbe=" << (properties.colourARGB.has_value() ? juce::String::toHexString ((int) *properties.colourARGB)
                                                               : juce::String ("(nicht gemeldet)"));
    logEvent ("SPUR", details);

    if (properties.name.has_value() && properties.name->isNotEmpty())
        trackName = *properties.name;
}

juce::String NoniusProbeProcessor::getLogPath() const
{
    return log != nullptr ? log->file.getFullPathName() : juce::String();
}

juce::String NoniusProbeProcessor::getStatusText() const
{
    return lastStatusLine;
}

void NoniusProbeProcessor::addMarker()
{
    ++markerCount;
    logEvent ("MARKER", juce::String (markerCount));
}

void NoniusProbeProcessor::logEditorVisibility (bool visible)
{
    logEvent (visible ? "FENSTER_SICHTBAR" : "FENSTER_UNSICHTBAR");
}

void NoniusProbeProcessor::logEvent (const juce::String& event, const juce::String& details)
{
    if (log != nullptr)
        log->write (event, details);
}

juce::AudioProcessorEditor* NoniusProbeProcessor::createEditor()
{
    return new NoniusProbeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoniusProbeProcessor();
}
