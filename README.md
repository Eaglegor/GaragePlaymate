# GaragePlaymate

> **Under development.** GaragePlaymate is not yet ready for end users. Core features described in the product docs are being implemented; see the [task index](docs/tasks/README.md) for progress.

GaragePlaymate is a Windows desktop multitrack practice player for recreational band members. Each playback session randomly selects one pre-recorded take per instrument, optionally simulates a technical failure (a track drops until the next song section), and records playback history for replay. See [docs/PRODUCT_DESIGN.md](docs/PRODUCT_DESIGN.md) for the full product specification and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system design and implementation guidance.

**License:** [GPL-3.0](LICENSE). Third-party attributions: [NOTICE](NOTICE).

## Prerequisites

- Windows 10 or 11 (x64)
- [Visual Studio 2026](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload
- [CMake](https://cmake.org/download/) 3.22 or newer
- Git (for CMake to fetch JUCE)
- **Optional (ASIO builds):** [Steinberg ASIO SDK](https://www.steinberg.net/asiosdk) — license acceptance required. Set `GARAGEPLAYMATE_ASIO_SDK_PATH`, the `ASIO_SDK_PATH` environment variable, or extract the SDK to `third_party/asio-sdk/`. Use `-DGARAGEPLAYMATE_ENABLE_ASIO=OFF` for WASAPI-only builds without the SDK.

## Build

From the repository root:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

The executable is written to:

```text
build/bin/Release/GaragePlaymate.exe
```

Other configurations use the same layout, for example `build/bin/Debug/GaragePlaymate.exe`.

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `GARAGEPLAYMATE_PORTABLE_BUILD` | `OFF` | When `ON`, the default data root is `{exe_dir}/data/` (portable distribution). |
| `GARAGEPLAYMATE_ENABLE_ASIO` | `ON` | Enable ASIO audio driver support (requires Steinberg ASIO SDK at configure time). |
| `GARAGEPLAYMATE_BUILD_TESTS` | `ON` | Build Catch2 unit tests (`GaragePlaymateTests`); run them with `ctest --test-dir build -C Release`. |

Example — portable, WASAPI-only configure:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64 `
  -DGARAGEPLAYMATE_PORTABLE_BUILD=ON `
  -DGARAGEPLAYMATE_ENABLE_ASIO=OFF
cmake --build build --config Release
```

## Run

After a Release build:

```powershell
.\build\bin\Release\GaragePlaymate.exe
```

The app currently opens an empty main window; library and playback features are being added task by task (see [docs/tasks](docs/tasks/README.md)).

## Song data

GaragePlaymate discovers songs from folders under your data root. Each song is a directory containing a `song.yaml` manifest and per-instrument WAV takes under `tracks/{track-id}/`. See [Product Design §6.2](docs/PRODUCT_DESIGN.md#62-song-data-model-folder-contract) for the folder contract.

Default data root (before user override):

| Build | Default data root |
|-------|-------------------|
| Portable (`GARAGEPLAYMATE_PORTABLE_BUILD=ON`) | `{exe_dir}/data/` |
| Installed (default) | `%USERPROFILE%/Documents/GaragePlaymate/` |

The data root can be overridden in Settings (for example to a OneDrive folder); see [Product Design §6.7](docs/PRODUCT_DESIGN.md#67-settings-and-distribution-modes).

App metadata — setlists, settings, playback history and disabled takes — is stored in a SQLite database at `{data root}/garageplaymate.db`, next to the `songs/` folder, so a portable install or a synced data folder carries its history along. Song folders themselves are never modified.

## Platform scope

v1 targets **Windows x64 only**. macOS and Linux are out of scope for the initial release.
