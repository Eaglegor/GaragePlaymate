# Agent Task Index

Run tasks **in numeric order** unless dependencies from a prior task are already satisfied.

**Shared context (read first):**
- [ARCHITECTURE.md](../ARCHITECTURE.md) — layers, interfaces, threading, conventions
- [PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md) — full product requirements

**Target agents:** Composer 2.5 or Cursor Auto — one task per session, manual sequential execution.

Each task has a **specification file** in this directory (scope, requirements, acceptance criteria) and a matching **GitHub issue** for tracking and PR linking.

**Issue tracker:** https://github.com/Eaglegor/GaragePlaymate/issues

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

| Task | Spec | Issue | Epic |
|------|------|-------|------|
| 01 | [TASK_01_cmake_juce_scaffold.md](TASK_01_cmake_juce_scaffold.md) | [#3](https://github.com/Eaglegor/GaragePlaymate/issues/3) | E1 |
| 02 | [TASK_02_license_notice_readme.md](TASK_02_license_notice_readme.md) | [#2](https://github.com/Eaglegor/GaragePlaymate/issues/2) | E1 |
| 03 | [TASK_03_application_shell.md](TASK_03_application_shell.md) | [#1](https://github.com/Eaglegor/GaragePlaymate/issues/1) | E1 |
| 04 | [TASK_04_core_domain_types.md](TASK_04_core_domain_types.md) | [#4](https://github.com/Eaglegor/GaragePlaymate/issues/4) | E2 |
| 05 | [TASK_05_song_yaml_parser.md](TASK_05_song_yaml_parser.md) | [#5](https://github.com/Eaglegor/GaragePlaymate/issues/5) | E2 |
| 06 | [TASK_06_song_folder_scanner.md](TASK_06_song_folder_scanner.md) | [#6](https://github.com/Eaglegor/GaragePlaymate/issues/6) | E2 |
| 07 | [TASK_07_sqlite_schema_migrations.md](TASK_07_sqlite_schema_migrations.md) | [#7](https://github.com/Eaglegor/GaragePlaymate/issues/7) | E2 |
| 08 | [TASK_08_data_path_resolver.md](TASK_08_data_path_resolver.md) | [#8](https://github.com/Eaglegor/GaragePlaymate/issues/8) | E2 |
| 09 | [TASK_09_audio_device_service.md](TASK_09_audio_device_service.md) | [#9](https://github.com/Eaglegor/GaragePlaymate/issues/9) | E3 |
| 10 | [TASK_10_streaming_track_source.md](TASK_10_streaming_track_source.md) | [#10](https://github.com/Eaglegor/GaragePlaymate/issues/10) | E3 |
| 11 | [TASK_11_preloaded_track_source.md](TASK_11_preloaded_track_source.md) | [#11](https://github.com/Eaglegor/GaragePlaymate/issues/11) | E3 |
| 12 | [TASK_12_multitrack_mixer_engine.md](TASK_12_multitrack_mixer_engine.md) | [#12](https://github.com/Eaglegor/GaragePlaymate/issues/12) | E3 |
| 13 | [TASK_13_transport_controller.md](TASK_13_transport_controller.md) | [#13](https://github.com/Eaglegor/GaragePlaymate/issues/13) | E3 |
| 14 | [TASK_14_section_navigator.md](TASK_14_section_navigator.md) | [#14](https://github.com/Eaglegor/GaragePlaymate/issues/14) | E4 |
| 15 | [TASK_15_section_seek_integration.md](TASK_15_section_seek_integration.md) | [#15](https://github.com/Eaglegor/GaragePlaymate/issues/15) | E4 |
| 16 | [TASK_16_random_take_selector.md](TASK_16_random_take_selector.md) | [#16](https://github.com/Eaglegor/GaragePlaymate/issues/16) | E5 |
| 17 | [TASK_17_session_persistence.md](TASK_17_session_persistence.md) | [#17](https://github.com/Eaglegor/GaragePlaymate/issues/17) | E5 |
| 18 | [TASK_18_playback_service_orchestration.md](TASK_18_playback_service_orchestration.md) | [#18](https://github.com/Eaglegor/GaragePlaymate/issues/18) | E5 |
| 19 | [TASK_19_failure_simulator_core.md](TASK_19_failure_simulator_core.md) | [#19](https://github.com/Eaglegor/GaragePlaymate/issues/19) | E6 |
| 20 | [TASK_20_failure_audio_integration.md](TASK_20_failure_audio_integration.md) | [#20](https://github.com/Eaglegor/GaragePlaymate/issues/20) | E6 |
| 21 | [TASK_21_library_service.md](TASK_21_library_service.md) | [#21](https://github.com/Eaglegor/GaragePlaymate/issues/21) | E7 |
| 22 | [TASK_22_setlist_crud.md](TASK_22_setlist_crud.md) | [#22](https://github.com/Eaglegor/GaragePlaymate/issues/22) | E7 |
| 23 | [TASK_23_settings_service.md](TASK_23_settings_service.md) | [#23](https://github.com/Eaglegor/GaragePlaymate/issues/23) | E7 |
| 24 | [TASK_24_library_ui.md](TASK_24_library_ui.md) | [#24](https://github.com/Eaglegor/GaragePlaymate/issues/24) | E7 |
| 25 | [TASK_25_song_detail_ui.md](TASK_25_song_detail_ui.md) | [#25](https://github.com/Eaglegor/GaragePlaymate/issues/25) | E7 |
| 26 | [TASK_26_session_bar_ui.md](TASK_26_session_bar_ui.md) | [#26](https://github.com/Eaglegor/GaragePlaymate/issues/26) | E7 |
| 27 | [TASK_27_history_panel_ui.md](TASK_27_history_panel_ui.md) | [#27](https://github.com/Eaglegor/GaragePlaymate/issues/27) | E7 |
| 28 | [TASK_28_settings_ui.md](TASK_28_settings_ui.md) | [#28](https://github.com/Eaglegor/GaragePlaymate/issues/28) | E7 |
| 29 | [TASK_29_keyboard_shortcuts.md](TASK_29_keyboard_shortcuts.md) | [#29](https://github.com/Eaglegor/GaragePlaymate/issues/29) | E8 |
| 30 | [TASK_30_sample_song_fixtures.md](TASK_30_sample_song_fixtures.md) | [#30](https://github.com/Eaglegor/GaragePlaymate/issues/30) | E8 |
| 31 | [TASK_31_core_unit_tests.md](TASK_31_core_unit_tests.md) | [#31](https://github.com/Eaglegor/GaragePlaymate/issues/31) | E8 |
| 32 | [TASK_32_packaging_portable_installer.md](TASK_32_packaging_portable_installer.md) | [#32](https://github.com/Eaglegor/GaragePlaymate/issues/32) | E8 |
| 33 | [TASK_33_mute_solo_controls.md](TASK_33_mute_solo_controls.md) | [#33](https://github.com/Eaglegor/GaragePlaymate/issues/33) | Optional |

## Dispatching a task to an agent

Copy this preamble into each agent session:

```
You are implementing a single task for GaragePlaymate (Windows desktop multitrack practice app).

Read shared context first:
- docs/ARCHITECTURE.md
- docs/PRODUCT_DESIGN.md (relevant sections cited in the task)

Implement ONLY the task below. Do not implement future tasks.
Match existing code conventions. Minimize scope.

Task spec: docs/tasks/TASK_XX_....md
Issue: https://github.com/Eaglegor/GaragePlaymate/issues/N
```
