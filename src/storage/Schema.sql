-- GaragePlaymate application database schema (ARCHITECTURE.md §8).
-- Applied idempotently on every open; bump kSchemaVersion in SQLiteStore.cpp
-- and add a migration step when changing existing tables.

-- settings: key → JSON value
CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

-- setlists
CREATE TABLE IF NOT EXISTS setlists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    created_at_ms INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS setlist_songs (
    setlist_id INTEGER NOT NULL REFERENCES setlists(id) ON DELETE CASCADE,
    song_id TEXT NOT NULL,
    sort_order INTEGER NOT NULL,
    PRIMARY KEY (setlist_id, song_id)
);

-- playback sessions (max 100 per song enforced in application code)
CREATE TABLE IF NOT EXISTS sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    song_id TEXT NOT NULL,
    timestamp_ms INTEGER NOT NULL,
    is_replay INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_sessions_song_time ON sessions (song_id, timestamp_ms);

CREATE TABLE IF NOT EXISTS session_takes (
    session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    track_id TEXT NOT NULL,
    take_filename TEXT NOT NULL,
    PRIMARY KEY (session_id, track_id)
);

CREATE TABLE IF NOT EXISTS failure_events (
    session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    track_id TEXT NOT NULL,
    onset_ms INTEGER NOT NULL,
    restore_ms INTEGER NOT NULL,
    failure_level REAL NOT NULL
);

-- user-disabled takes (excluded from random pool)
CREATE TABLE IF NOT EXISTS disabled_takes (
    song_id TEXT NOT NULL,
    track_id TEXT NOT NULL,
    take_filename TEXT NOT NULL,
    PRIMARY KEY (song_id, track_id, take_filename)
);
