# GaragePlaymate — Architecture Reference

**Version:** v1.0 (Windows desktop)  
**Audience:** Development agents implementing tasks tracked as [GitHub issues](https://github.com/Eaglegor/GaragePlaymate/issues) ([task index](tasks/README.md))  
**Product spec:** [PRODUCT_DESIGN.md](PRODUCT_DESIGN.md)

This document is the **shared context** for all implementation tasks. Read it before starting any task.

---

## 1. Product summary

GaragePlaymate is a Windows desktop multitrack practice player for recreational band members. Unlike fixed backing tracks, each Play session randomly selects one pre-recorded take per instrument, optionally simulates a technical failure (track drops until the next section), and records playback history for replay.

**v1 scope:** Folder-based song library, setlists, section navigation, random takes, session history (100/song), one failure mode, WAV-only, WASAPI default with optional ASIO. No EQ, VST, recording, cloud sync, or alignment validation.

---

## 2. Design principles

1. **UI/engine separation** — Domain logic in `src/core/` must not depend on JUCE UI modules. Core may use the C++ standard library only. Audio code in `src/audio/` may use JUCE audio modules but not UI.
2. **Audio thread safety (NFR-REL-01)** — No heap allocations, locks, or I/O on the audio callback thread after playback starts. Preload allocation happens on the message/loader thread before `start()`.
3. **Folder contract is source of truth** — Songs and takes live on disk under the user-configurable data root. App metadata (setlists, settings, session history, disabled takes) lives in SQLite, never inside song folders.
4. **WASAPI-first (NFR-UX-01)** — First launch uses the system default WASAPI output with no configuration. ASIO is optional.
5. **No alignment validation in v1** — All takes start at file position `00:00`. Variable take endings are supported. The archivist trims files correctly; the app does not detect misalignment.

---

## 3. Layered architecture

```
┌─────────────────────────────────────────────────────────┐
│  ui/          JUCE Components (Library, SongDetail…)   │
├─────────────────────────────────────────────────────────┤
│  app/         AppController — wires UI ↔ services        │
├─────────────────────────────────────────────────────────┤
│  services/    LibraryService, PlaybackService, Settings  │
├───────────────┬─────────────────────────────────────────┤
│  core/        │  audio/                                  │
│  (pure logic) │  MultitrackMixer, TrackSources, Transport│
├───────────────┴─────────────────────────────────────────┤
│  storage/     SongYamlParser, SongFolderScanner, SQLite  │
└─────────────────────────────────────────────────────────┘
```

### Dependency rules

| Layer | May depend on | Must NOT depend on |
|-------|---------------|-------------------|
| `core/` | C++ STL | JUCE, SQLite, yaml-cpp |
| `storage/` | `core/`, yaml-cpp, SQLite | JUCE UI |
| `audio/` | `core/`, JUCE audio | JUCE GUI |
| `services/` | `core/`, `storage/`, `audio/` | JUCE GUI (use callbacks/interfaces for UI) |
| `ui/` | `services/`, JUCE GUI | Direct SQLite or scanner calls |
| `app/` | All layers | — |

---

## 4. Repository layout

```
GaragePlaymate/
├── CMakeLists.txt
├── LICENSE                          # GPL-3.0
├── NOTICE                           # Third-party notices
├── cmake/
│   ├── FetchJuce.cmake
│   └── CompilerWarnings.cmake
├── docs/
│   ├── PRODUCT_DESIGN.md
│   ├── ARCHITECTURE.md                # this file
│   └── tasks/                         # task index → GitHub issues
├── src/
│   ├── main.cpp
│   ├── app/
│   ├── core/
│   ├── storage/
│   ├── audio/
│   ├── services/
│   └── ui/
├── tests/
│   └── core/
├── data/                              # dev sample songs
└── packaging/
```

---

## 5. Domain types (`src/core/`)

### 5.1 Song model (`SongTypes.h`)

```cpp
struct TakeFile {
    std::string filename;       // e.g. "take-01.wav"
    std::filesystem::path absolutePath;
    int64_t durationMs = 0;     // from WAV header; 0 if unreadable
    bool disabled = false;        // runtime flag from SQLite disabled_takes
};

struct TrackDef {
    std::string id;               // e.g. "drums"
    std::string name;             // display name
    std::filesystem::path folderPath;
    float defaultVolume = 1.0f;   // linear 0.0–1.0
    std::vector<TakeFile> takes;
};

struct Section {
    std::string id;
    std::string name;
    int64_t startMs = 0;
};

struct Song {
    std::string id;
    std::string title;
    std::optional<int> bpm;
    std::optional<std::pair<int,int>> timeSignature;  // [numerator, denominator]
    std::filesystem::path folderPath;
    std::vector<TrackDef> tracks;
    std::vector<Section> sections;  // sorted ascending by startMs
};
```

### 5.2 Session model (`SessionTypes.h`)

```cpp
struct FailureEvent {
    std::string trackId;
    int64_t onsetMs = 0;
    int64_t restoreMs = 0;      // next section startMs or session end
    float failureLevel = 0.15f; // linear gain during failure
};

struct PlaybackSession {
    int64_t id = 0;             // SQLite row id
    std::string songId;
    int64_t timestampUnixMs = 0;
    std::map<std::string, std::string> takeMap;  // trackId → take filename
    std::vector<FailureEvent> failureEvents;
    bool isReplay = false;
};
```

### 5.3 Settings model (persisted via SettingsService)

```cpp
enum class AudioDriverType { Wasapi, Asio };
enum class AudioLoadingMode { Stream, Preload };

struct AppSettings {
    std::optional<std::filesystem::path> dataRootOverride;
    AudioDriverType driverType = AudioDriverType::Wasapi;
    std::string outputDeviceId;           // JUCE device identifier
    int bufferSizeSamples = 512;
    AudioLoadingMode loadingMode = AudioLoadingMode::Stream;
    bool failureSimulationEnabled = false;
    float failureProbability = 0.10f;   // 0.0–1.0
    float failureLevel = 0.15f;           // 0.0–0.5
    int failureFadeMs = 300;
};
```

---

## 6. On-disk song folder contract

```
{DataRoot}/
  songs/
    {song-id}/
      song.yaml              # required manifest
      tracks/
        {track-id}/
          take-01.wav
          take-02.wav
```

### Minimum `song.yaml` fields

```yaml
id: sweet-child-o-mine
title: Sweet Child O' Mine
bpm: 120                    # optional
timeSignature: [4, 4]       # optional
tracks:
  - id: drums
    name: Drums
    folder: tracks/drums
    defaultVolume: 1.0
sections:
  - id: intro
    name: Intro
    startMs: 0
  - id: verse1
    name: Verse 1
    startMs: 15300
```

Sections may also live in a separate `sections.yaml` (optional; defer unless needed — inline in `song.yaml` is sufficient for v1).

### Supported audio (v1)

- **WAV only:** PCM 16/24/32-bit, 44.1 kHz or 48 kHz
- Non-WAV files in track folders: ignore with warning log; do not fail scan

---

## 7. Data path resolution (`DataPathResolver`)

Compile-time flag: `GARAGEPLAYMATE_PORTABLE_BUILD`

| Build | Default data root |
|-------|-------------------|
| Portable | `{exe_dir}/data/` |
| Installed | `%USERPROFILE%/Documents/GaragePlaymate/` |

Effective root: `settings.dataRootOverride.value_or(defaultRoot)`  
Songs scanned at: `{effectiveRoot}/songs/{song-id}/`

User config DB location: `{effectiveRoot}/garageplaymate.db` (or `%APPDATA%/GaragePlaymate/garageplaymate.db` — pick one and document in README; recommend co-located with data root for portable simplicity).

---

## 8. SQLite schema (`src/storage/Schema.sql`)

```sql
-- settings: key → JSON value
CREATE TABLE settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

-- setlists
CREATE TABLE setlists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    created_at_ms INTEGER NOT NULL
);

CREATE TABLE setlist_songs (
    setlist_id INTEGER NOT NULL REFERENCES setlists(id) ON DELETE CASCADE,
    song_id TEXT NOT NULL,
    sort_order INTEGER NOT NULL,
    PRIMARY KEY (setlist_id, song_id)
);

-- playback sessions (max 100 per song enforced in application code)
CREATE TABLE sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    song_id TEXT NOT NULL,
    timestamp_ms INTEGER NOT NULL,
    is_replay INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE session_takes (
    session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    track_id TEXT NOT NULL,
    take_filename TEXT NOT NULL,
    PRIMARY KEY (session_id, track_id)
);

CREATE TABLE failure_events (
    session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    track_id TEXT NOT NULL,
    onset_ms INTEGER NOT NULL,
    restore_ms INTEGER NOT NULL,
    failure_level REAL NOT NULL
);

-- user-disabled takes (excluded from random pool)
CREATE TABLE disabled_takes (
    song_id TEXT NOT NULL,
    track_id TEXT NOT NULL,
    take_filename TEXT NOT NULL,
    PRIMARY KEY (song_id, track_id, take_filename)
);
```

---

## 9. Audio engine

### 9.1 Gain model

Use **linear gain 0.0–1.0** internally everywhere. UI sliders map directly to linear values. Optional dB display in UI is cosmetic only.

### 9.2 `ITrackAudioSource` interface

```cpp
class ITrackAudioSource {
public:
    virtual ~ITrackAudioSource() = default;
    virtual bool prepare(const std::filesystem::path& wavPath) = 0;
    virtual void release() = 0;
    virtual void getNextAudioBlock(juce::AudioBuffer<float>& buffer,
                                   int startSample, int numSamples) = 0;
    virtual int64_t getLengthSamples() const = 0;
    virtual int getNumChannels() const = 0;
    virtual double getSampleRate() const = 0;
    virtual bool isFinished() const = 0;
    virtual void setGain(float linearGain) = 0;
    virtual void seekToSample(int64_t samplePos) = 0;
};
```

- **`StreamingTrackSource`:** reads via `juce::AudioFormatReader` during playback (FR-PLAY-09 Stream mode)
- **`PreloadedTrackSource`:** loads entire take into `juce::AudioBuffer<float>` on message thread before start (FR-PLAY-09 Preload mode)

Both implementations must be **real-time safe after `prepare()` completes**.

### 9.3 `MultitrackMixerEngine`

- Accepts N `ITrackAudioSource` instances
- All tracks start simultaneously at sample 0 (FR-PLAY-01)
- Session length = max `getLengthSamples()` across active takes (FR-PLAY-04)
- When a track finishes, output silence for that track; others continue (FR-PLAY-05)
- Per-track linear gain; failure simulator may override gain per track per sample block

### 9.4 `TransportController`

States: `Stopped`, `Playing`, `Paused`  
Operations: `play()`, `pause()`, `stop()`, `seekToMs(int64_t)`  
Emits position callbacks on message thread (timer or `AsyncUpdater`, not from audio thread directly).

### 9.5 `AudioDeviceService`

- Wraps `juce::AudioDeviceManager`
- First launch: WASAPI, system default output, buffer 512 (FR-PLAY-07)
- Exposes device list for WASAPI and ASIO (FR-PLAY-06)
- ASIO unavailable → fall back to last WASAPI device + user notification (FR-PLAY-08)

---

## 10. Core domain services

### 10.1 `RandomTakeSelector`

- Input: `Song`, set of disabled `(songId, trackId, takeFilename)`
- Output: `map<trackId, takeFilename>` — one uniform random take per track from non-disabled pool
- Empty pool for a track → error; caller must handle (skip song or show message)
- Seeding: use `std::random_device` / Mersenne Twister; injectable RNG for tests

### 10.2 `SectionNavigator`

Pure functions on `Song.sections` and current position ms:
- `getCurrentSection(sections, positionMs) → Section`
- `getNextSection(sections, positionMs) → optional<Section>`
- `getPreviousSection(sections, positionMs) → optional<Section>`
- `seekTargetMs(sectionId) → startMs`

Sections sorted ascending by `startMs`. Last section extends to session end.

### 10.3 `FailureSimulator`

At play start (Bernoulli trial with probability `p`):
1. If not triggered → no events
2. Pick random track
3. Pick random onset time `t` within playable window (exclude last section; exclude last N seconds — use constant `kFailureExcludeTailMs = 5000`)
4. Restore at `sections[n+1].startMs` (next section boundary)
5. If failure would occur in last section → skip failure entirely (FR-FAIL-05)

During playback, `applyGain(trackId, baseGain, positionMs) → float` applies fade over `fadeMs` (default 300ms) down to `failureLevel`, holds until restore, then fades back.

---

## 11. Application services

### 11.1 `LibraryService`

- `scanLibrary()` — walks `{dataRoot}/songs/`, parses manifests, enumerates WAV takes
- Called on startup and on user Rescan (FR-LIB-01)
- Returns `vector<SongSummary>` for UI: title, artist optional, bpm, track count, take counts (FR-LIB-02)
- Tolerates cloud-sync file locks: skip unreadable files, log warning, continue scan

### 11.2 `PlaybackService`

Orchestrates one playback session:

```
Play(songId, optional<replaySessionId>):
  1. Load song from library cache
  2. If replay → load takeMap from SQLite; else RandomTakeSelector + FailureSimulator.schedule
  3. Prepare track sources (Stream or Preload per settings)
  4. Bind to MultitrackMixerEngine; start transport at 00:00
  5. On stop/end → persist PlaybackSession to SQLite; prune to 100/song
```

Changing takes mid-playback requires Stop → Play (FR-RAND-02).

### 11.3 `SettingsService`

- Load/save `AppSettings` to SQLite `settings` table as JSON blobs
- Resolve effective data root via `DataPathResolver`
- Notify listeners on change (data path, audio device, loading mode)

---

## 12. Threading model

| Thread | Work |
|--------|------|
| **Message (JUCE main)** | UI, library scan, SQLite, YAML parse, preload allocation, transport state |
| **Audio (JUCE callback)** | Mix buffers, apply gain automation, read preloaded/stream buffers |
| **Background (optional)** | Library rescan worker; post results to message thread via `MessageManager::callAsync` |

**Rules:**
- Audio callback: no `new`/`malloc`, no mutex locks, no file I/O, no SQLite
- UI updates from audio: use `AsyncUpdater` or atomic flags polled on timer
- Preload progress: callback on message thread during `prepare()`

---

## 13. UI structure (v1)

| Component | Responsibility |
|-----------|----------------|
| `MainWindow` | Hosts panels, menu bar, keyboard focus |
| `LibraryPanel` | Song list, setlist sidebar, search |
| `SongDetailPanel` | Section strip, volume sliders, take indicators, Play/Stop, preload bar |
| `SessionBar` | Now playing, current section, elapsed time, failure indicator |
| `HistoryPanel` | Session list per song, Replay button |
| `SettingsPanel` | Data path picker, audio, loading mode, failure defaults, Rescan |

**Keyboard shortcuts:** Space = Play/Pause, Left/Right = prev/next section, R = rescan

---

## 14. Build and technology

| Item | Choice |
|------|--------|
| Language | C++20 |
| Framework | JUCE 8 (GUI + audio) |
| Song metadata | yaml-cpp |
| Persistence | SQLite (amalgamation or FetchContent) |
| Tests | Catch2 for `src/core/` and parser tests |
| Build | CMake 3.22+, MSVC 2026 (Visual Studio 2026), x64 |
| License | GPL-3.0 |

### CMake options

```cmake
option(GARAGEPLAYMATE_PORTABLE_BUILD "Portable build uses ./data/ default path" OFF)
option(GARAGEPLAYMATE_ENABLE_ASIO "Enable ASIO audio driver" ON)
option(GARAGEPLAYMATE_BUILD_TESTS "Build unit tests" ON)
```

### JUCE modules (minimum)

`juce_core`, `juce_events`, `juce_data_structures`, `juce_gui_basics`, `juce_gui_extra`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`, `juce_audio_utils`

---

## 15. Coding conventions

- **Namespace:** `garageplaymate::` for all project code
- **Naming:** PascalCase classes, camelCase methods, snake_case for SQL and YAML keys matching manifest
- **Paths:** `std::filesystem::path`; UTF-8 on Windows via JUCE `File` where needed
- **Errors:** Use `std::optional` or `Result`-like pattern for recoverable errors; `juce::Logger::writeToLog` for diagnostics
- **Headers:** `#pragma once` preferred
- **Tests:** Place in `tests/core/`, name `test_<component>.cpp`

---

## 16. Archivist workflow (documentation only — not implemented)

1. Create `{song-id}/` folder under `{DataRoot}/songs/`
2. Write `song.yaml` with tracks and sections
3. Export WAV takes per instrument; trim so bar 0 is at file start
4. Takes may end at different lengths — this is intentional
5. Rescan library in app (or restart app)

---

## 17. Task index

Run tasks in numeric order. Each task is a [GitHub issue](https://github.com/Eaglegor/GaragePlaymate/issues); see [docs/tasks/README.md](tasks/README.md) for issue numbers and batches.

| Task | Issue | Summary |
|------|-------|---------|
| TASK-01 | [#3](https://github.com/Eaglegor/GaragePlaymate/issues/3) | CMake + JUCE scaffold |
| TASK-02 | [#2](https://github.com/Eaglegor/GaragePlaymate/issues/2) | LICENSE, NOTICE, README |
| TASK-03 | [#1](https://github.com/Eaglegor/GaragePlaymate/issues/1) | Application shell |
| TASK-04 | [#4](https://github.com/Eaglegor/GaragePlaymate/issues/4) | Core domain types |
| TASK-05 | [#5](https://github.com/Eaglegor/GaragePlaymate/issues/5) | Song YAML parser |
| TASK-06 | [#6](https://github.com/Eaglegor/GaragePlaymate/issues/6) | Song folder scanner |
| TASK-07 | [#7](https://github.com/Eaglegor/GaragePlaymate/issues/7) | SQLite schema + store |
| TASK-08 | [#8](https://github.com/Eaglegor/GaragePlaymate/issues/8) | Data path resolver |
| TASK-09 | [#9](https://github.com/Eaglegor/GaragePlaymate/issues/9) | Audio device service |
| TASK-10 | [#10](https://github.com/Eaglegor/GaragePlaymate/issues/10) | Streaming track source |
| TASK-11 | [#11](https://github.com/Eaglegor/GaragePlaymate/issues/11) | Preloaded track source |
| TASK-12 | [#12](https://github.com/Eaglegor/GaragePlaymate/issues/12) | Multitrack mixer engine |
| TASK-13 | [#13](https://github.com/Eaglegor/GaragePlaymate/issues/13) | Transport controller |
| TASK-14 | [#14](https://github.com/Eaglegor/GaragePlaymate/issues/14) | Section navigator |
| TASK-15 | [#15](https://github.com/Eaglegor/GaragePlaymate/issues/15) | Section seek integration |
| TASK-16 | [#16](https://github.com/Eaglegor/GaragePlaymate/issues/16) | Random take selector |
| TASK-17 | [#17](https://github.com/Eaglegor/GaragePlaymate/issues/17) | Session persistence |
| TASK-18 | [#18](https://github.com/Eaglegor/GaragePlaymate/issues/18) | Playback service orchestration |
| TASK-19 | [#19](https://github.com/Eaglegor/GaragePlaymate/issues/19) | Failure simulator core |
| TASK-20 | [#20](https://github.com/Eaglegor/GaragePlaymate/issues/20) | Failure audio integration |
| TASK-21 | [#21](https://github.com/Eaglegor/GaragePlaymate/issues/21) | Library service |
| TASK-22 | [#22](https://github.com/Eaglegor/GaragePlaymate/issues/22) | Setlist CRUD |
| TASK-23 | [#23](https://github.com/Eaglegor/GaragePlaymate/issues/23) | Settings service |
| TASK-24 | [#24](https://github.com/Eaglegor/GaragePlaymate/issues/24) | Library UI |
| TASK-25 | [#25](https://github.com/Eaglegor/GaragePlaymate/issues/25) | Song detail UI |
| TASK-26 | [#26](https://github.com/Eaglegor/GaragePlaymate/issues/26) | Session bar UI |
| TASK-27 | [#27](https://github.com/Eaglegor/GaragePlaymate/issues/27) | History panel UI |
| TASK-28 | [#28](https://github.com/Eaglegor/GaragePlaymate/issues/28) | Settings UI |
| TASK-29 | [#29](https://github.com/Eaglegor/GaragePlaymate/issues/29) | Keyboard shortcuts |
| TASK-30 | [#30](https://github.com/Eaglegor/GaragePlaymate/issues/30) | Sample song fixtures |
| TASK-31 | [#31](https://github.com/Eaglegor/GaragePlaymate/issues/31) | Core unit tests |
| TASK-32 | [#32](https://github.com/Eaglegor/GaragePlaymate/issues/32) | Packaging (portable + installer) |
| TASK-33 | [#33](https://github.com/Eaglegor/GaragePlaymate/issues/33) | Mute/solo (optional) |

---

## 18. Acceptance criteria reference

See [PRODUCT_DESIGN.md §13](PRODUCT_DESIGN.md) for full acceptance tests. Critical paths:

1. Valid song folder → appears after rescan
2. Fresh install + song folder → plays via WASAPI without opening Settings
3. Random takes differ across plays; replay reproduces exact combination
4. Failure at 100% probability → track drops before next section, restores at boundary
5. Variable take lengths → shorter tracks silent, session ends at longest take
6. Preload mode → progress bar, glitch-free playback, memory released on Stop
7. Session history capped at 100 per song
