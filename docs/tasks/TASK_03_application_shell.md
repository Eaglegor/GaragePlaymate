# TASK-03: Application Shell

**Epic:** E1 — Project scaffold  
**Depends on:** TASK_01, TASK_02  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Replace the stub `main.cpp` with a JUCE `Application` subclass and an empty `MainWindow` with menu bar stub, establishing the app entry point and UI shell.

## Requirements (from PDD)

- JUCE native UI (§9.1)
- UI outline: minimal screens to be added later (§8)

## Scope

### Create/modify

- `src/main.cpp` — `START_JUCE_APPLICATION(GaragePlaymateApplication)`
- `src/app/Application.h` / `Application.cpp` — `juce::JUCEApplication` subclass
- `src/app/AppController.h` — empty class declaration (wired in later tasks)
- `src/ui/MainWindow.h` / `MainWindow.cpp` — `juce::DocumentWindow` or `juce::ResizableWindow`
- Update root `CMakeLists.txt` to compile new source files

### Do NOT

- Implement library, playback, or settings panels
- Wire services or SQLite
- Add keyboard shortcuts (TASK_29)

## Implementation notes

1. Namespace: `garageplaymate::` per ARCHITECTURE §15.
2. `Application::initialise()`:
   - Create `AppController` (empty for now)
   - Create and show `MainWindow` titled "GaragePlaymate"
   - Default window size ~1280×800, centred
3. `Application::shutdown()` — destroy window and controller in reverse order.
4. `MainWindow`:
   - Menu bar with placeholder menus: File (Exit), View (stub), Help (About stub showing version "0.1.0-dev")
   - Central component: `juce::Label` with text "GaragePlaymate — UI coming soon" or empty `Component`
   - `closeButtonPressed()` calls `JUCEApplication::quit()`
5. Use JUCE 8 recommended application macro and module includes.
6. Set application display name via `juce::JUCEApplicationBase::setApplicationName`.

## Verification

- [ ] App launches on Windows showing titled window
- [ ] File → Exit closes app cleanly
- [ ] Help → About shows version dialog
- [ ] Release build runs without console window (GUI app subsystem)
