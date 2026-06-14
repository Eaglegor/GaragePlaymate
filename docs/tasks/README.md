# Task Index (GitHub Issues)

Implementation tasks live as **GitHub issues**, not markdown files in this repo.

**Issue tracker:** https://github.com/Eaglegor/GaragePlaymate/issues

**Shared context (read before starting any task):**
- [ARCHITECTURE.md](../ARCHITECTURE.md) — layers, interfaces, threading, conventions
- [PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md) — full product requirements

Run tasks **in numeric order** unless dependencies from a prior task are already satisfied.

## Execution batches

| Batch | Tasks | Milestone |
|-------|-------|-----------|
| Foundation | TASK-01 – TASK-08 | Builds, scans song folders, SQLite ready |
| Audio spine | TASK-09 – TASK-15 | Multitrack playback + section seek |
| Smart playback | TASK-16 – TASK-20 | Random takes, history, failure sim |
| Product UI | TASK-21 – TASK-28 | Full musician-facing app |
| Ship | TASK-29 – TASK-32 | Shortcuts, fixtures, tests, packages |
| Optional | TASK-33 | Mute/solo if time permits |

## Task list

| Task | Issue | Epic |
|------|-------|------|
| TASK-01 | [#3](https://github.com/Eaglegor/GaragePlaymate/issues/3) | E1 |
| TASK-02 | [#2](https://github.com/Eaglegor/GaragePlaymate/issues/2) | E1 |
| TASK-03 | [#1](https://github.com/Eaglegor/GaragePlaymate/issues/1) | E1 |
| TASK-04 | [#4](https://github.com/Eaglegor/GaragePlaymate/issues/4) | E2 |
| TASK-05 | [#5](https://github.com/Eaglegor/GaragePlaymate/issues/5) | E2 |
| TASK-06 | [#6](https://github.com/Eaglegor/GaragePlaymate/issues/6) | E2 |
| TASK-07 | [#7](https://github.com/Eaglegor/GaragePlaymate/issues/7) | E2 |
| TASK-08 | [#8](https://github.com/Eaglegor/GaragePlaymate/issues/8) | E2 |
| TASK-09 | [#9](https://github.com/Eaglegor/GaragePlaymate/issues/9) | E3 |
| TASK-10 | [#10](https://github.com/Eaglegor/GaragePlaymate/issues/10) | E3 |
| TASK-11 | [#11](https://github.com/Eaglegor/GaragePlaymate/issues/11) | E3 |
| TASK-12 | [#12](https://github.com/Eaglegor/GaragePlaymate/issues/12) | E3 |
| TASK-13 | [#13](https://github.com/Eaglegor/GaragePlaymate/issues/13) | E3 |
| TASK-14 | [#14](https://github.com/Eaglegor/GaragePlaymate/issues/14) | E4 |
| TASK-15 | [#15](https://github.com/Eaglegor/GaragePlaymate/issues/15) | E4 |
| TASK-16 | [#16](https://github.com/Eaglegor/GaragePlaymate/issues/16) | E5 |
| TASK-17 | [#17](https://github.com/Eaglegor/GaragePlaymate/issues/17) | E5 |
| TASK-18 | [#18](https://github.com/Eaglegor/GaragePlaymate/issues/18) | E5 |
| TASK-19 | [#19](https://github.com/Eaglegor/GaragePlaymate/issues/19) | E6 |
| TASK-20 | [#20](https://github.com/Eaglegor/GaragePlaymate/issues/20) | E6 |
| TASK-21 | [#21](https://github.com/Eaglegor/GaragePlaymate/issues/21) | E7 |
| TASK-22 | [#22](https://github.com/Eaglegor/GaragePlaymate/issues/22) | E7 |
| TASK-23 | [#23](https://github.com/Eaglegor/GaragePlaymate/issues/23) | E7 |
| TASK-24 | [#24](https://github.com/Eaglegor/GaragePlaymate/issues/24) | E7 |
| TASK-25 | [#25](https://github.com/Eaglegor/GaragePlaymate/issues/25) | E7 |
| TASK-26 | [#26](https://github.com/Eaglegor/GaragePlaymate/issues/26) | E7 |
| TASK-27 | [#27](https://github.com/Eaglegor/GaragePlaymate/issues/27) | E7 |
| TASK-28 | [#28](https://github.com/Eaglegor/GaragePlaymate/issues/28) | E7 |
| TASK-29 | [#29](https://github.com/Eaglegor/GaragePlaymate/issues/29) | E8 |
| TASK-30 | [#30](https://github.com/Eaglegor/GaragePlaymate/issues/30) | E8 |
| TASK-31 | [#31](https://github.com/Eaglegor/GaragePlaymate/issues/31) | E8 |
| TASK-32 | [#32](https://github.com/Eaglegor/GaragePlaymate/issues/32) | E8 |
| TASK-33 | [#33](https://github.com/Eaglegor/GaragePlaymate/issues/33) | Optional |

## Dispatching a task to an agent

Copy this preamble into each agent session:

```
You are implementing a single task for GaragePlaymate (Windows desktop multitrack practice app).

Read shared context first:
- docs/ARCHITECTURE.md
- docs/PRODUCT_DESIGN.md (relevant sections cited in the issue)

Implement ONLY the GitHub issue below. Do not implement future tasks.
Match existing code conventions. Minimize scope.

Issue: https://github.com/Eaglegor/GaragePlaymate/issues/N
```
