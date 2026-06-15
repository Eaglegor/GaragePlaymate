# TASK-30: Sample Song Fixtures

**Epic:** E8 — Polish and packaging  
**Depends on:** TASK_05  
**Shared context:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md), [docs/PRODUCT_DESIGN.md](../PRODUCT_DESIGN.md)

## Goal

Create `data/songs/demo-song/` with valid `song.yaml`, section definitions, and minimal WAV takes for manual testing and acceptance tests.

## Requirements (from PDD)

- Acceptance tests #1, #5, #7, #13
- §6.2 folder contract

## Scope

### Create/modify

- `data/songs/demo-song/song.yaml`
- `data/songs/demo-song/tracks/drums/take-01.wav`, `take-02.wav`, `take-03.wav`
- `data/songs/demo-song/tracks/bass/take-01.wav`, `take-02.wav`
- `data/songs/demo-song/tracks/guitar/take-01.wav` (longer duration for variable-length test)
- `data/songs/demo-song/tracks/vocals/take-01.wav`
- `data/songs/demo-song/tracks/drums/readme.txt` or `dummy.mp3` — non-WAV to test warning
- `scripts/generate_demo_wavs.py` or `scripts/generate_demo_wavs.ps1` — generate silent/sine WAVs at 44.1kHz 16-bit

### Do NOT

- Commit large audio files (>1MB each); keep demo clips 5–30 seconds

## Implementation notes

1. `song.yaml` example:
   - id: `demo-song`
   - title: `Demo Song`
   - bpm: 120
   - 4 tracks: drums, bass, guitar, vocals
   - sections: intro 0, verse1 5000, chorus1 15000, outro 25000 ms

2. WAV generation script:
   - drums/bass/vocals takes: 30 seconds
   - guitar take-01: 35 seconds (variable length test #7)
   - Optional: embed quiet sine tone for audible verification

3. Document in README: copy `data/` to portable `./data/` or point settings to repo `data/`.

4. Add `.gitattributes` or note if binary WAVs committed.

## Verification

- [ ] Scanner finds demo-song with correct take counts
- [ ] Parser tests can reuse or mirror this fixture
- [ ] Playback runs 30–35 seconds without error
- [ ] Non-WAV in folder produces scan warning only
