# Agent Task Index

Run tasks **in numeric order** unless dependencies from a prior task are already satisfied.

**Shared context (read first):**
- [ARCHITECTURE.md](../ARCHITECTURE.md) — layers, interfaces, threading, conventions
- [PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md) — full product requirements

**Target agents:** Composer 2.5 or Cursor Auto — one task per session, manual sequential execution.

## Execution batches

| Batch | Tasks | Milestone |
|-------|-------|-----------|
| Foundation | TASK_01 – TASK_08 | Builds, scans song folders, SQLite ready |
| Audio spine | TASK_09 – TASK_15 | Multitrack playback + section seek |
| Smart playback | TASK_16 – TASK_20 | Random takes, history, failure sim |
| Product UI | TASK_21 – TASK_28 | Full musician-facing app |
| Ship | TASK_29 – TASK_32 | Shortcuts, fixtures, tests, packages |
| Optional | TASK_33 | Mute/solo if time permits |

## Task list

| Task | File | Epic |
|------|------|------|
| 01 | [TASK_01_cmake_juce_scaffold.md](TASK_01_cmake_juce_scaffold.md) | E1 |
| 02 | [TASK_02_license_notice_readme.md](TASK_02_license_notice_readme.md) | E1 |
| 03 | [TASK_03_application_shell.md](TASK_03_application_shell.md) | E1 |
| 04 | [TASK_04_core_domain_types.md](TASK_04_core_domain_types.md) | E2 |
| 05 | [TASK_05_song_yaml_parser.md](TASK_05_song_yaml_parser.md) | E2 |
| 06 | [TASK_06_song_folder_scanner.md](TASK_06_song_folder_scanner.md) | E2 |
| 07 | [TASK_07_sqlite_schema_migrations.md](TASK_07_sqlite_schema_migrations.md) | E2 |
| 08 | [TASK_08_data_path_resolver.md](TASK_08_data_path_resolver.md) | E2 |
| 09 | [TASK_09_audio_device_service.md](TASK_09_audio_device_service.md) | E3 |
| 10 | [TASK_10_streaming_track_source.md](TASK_10_streaming_track_source.md) | E3 |
| 11 | [TASK_11_preloaded_track_source.md](TASK_11_preloaded_track_source.md) | E3 |
| 12 | [TASK_12_multitrack_mixer_engine.md](TASK_12_multitrack_mixer_engine.md) | E3 |
| 13 | [TASK_13_transport_controller.md](TASK_13_transport_controller.md) | E3 |
| 14 | [TASK_14_section_navigator.md](TASK_14_section_navigator.md) | E4 |
| 15 | [TASK_15_section_seek_integration.md](TASK_15_section_seek_integration.md) | E4 |
| 16 | [TASK_16_random_take_selector.md](TASK_16_random_take_selector.md) | E5 |
| 17 | [TASK_17_session_persistence.md](TASK_17_session_persistence.md) | E5 |
| 18 | [TASK_18_playback_service_orchestration.md](TASK_18_playback_service_orchestration.md) | E5 |
| 19 | [TASK_19_failure_simulator_core.md](TASK_19_failure_simulator_core.md) | E6 |
| 20 | [TASK_20_failure_audio_integration.md](TASK_20_failure_audio_integration.md) | E6 |
| 21 | [TASK_21_library_service.md](TASK_21_library_service.md) | E7 |
| 22 | [TASK_22_setlist_crud.md](TASK_22_setlist_crud.md) | E7 |
| 23 | [TASK_23_settings_service.md](TASK_23_settings_service.md) | E7 |
| 24 | [TASK_24_library_ui.md](TASK_24_library_ui.md) | E7 |
| 25 | [TASK_25_song_detail_ui.md](TASK_25_song_detail_ui.md) | E7 |
| 26 | [TASK_26_session_bar_ui.md](TASK_26_session_bar_ui.md) | E7 |
| 27 | [TASK_27_history_panel_ui.md](TASK_27_history_panel_ui.md) | E7 |
| 28 | [TASK_28_settings_ui.md](TASK_28_settings_ui.md) | E7 |
| 29 | [TASK_29_keyboard_shortcuts.md](TASK_29_keyboard_shortcuts.md) | E8 |
| 30 | [TASK_30_sample_song_fixtures.md](TASK_30_sample_song_fixtures.md) | E8 |
| 31 | [TASK_31_core_unit_tests.md](TASK_31_core_unit_tests.md) | E8 |
| 32 | [TASK_32_packaging_portable_installer.md](TASK_32_packaging_portable_installer.md) | E8 |
| 33 | [TASK_33_mute_solo_controls.md](TASK_33_mute_solo_controls.md) | Optional |

## Dispatching a task to an agent

Copy this preamble into each agent session:

```
You are implementing a single task for GaragePlaymate (Windows desktop multitrack practice app).

Read shared context first:
- docs/ARCHITECTURE.md
- docs/PRODUCT_DESIGN.md (relevant sections cited in task)

Implement ONLY the task file below. Do not implement future tasks.
Match existing code conventions. Minimize scope.

Task: docs/tasks/TASK_XX_....md
```
