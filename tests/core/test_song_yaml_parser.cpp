#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "storage/SongYamlParser.h"

#include <filesystem>

namespace {

std::filesystem::path fixturesRoot() {
    return std::filesystem::path(TEST_FIXTURES_DIR) / "songs";
}

std::filesystem::path songYamlPath(const std::filesystem::path& songFolder) {
    return songFolder / "song.yaml";
}

const garageplaymate::TrackDef* findTrack(const garageplaymate::Song& song, const std::string& id) {
    for (const garageplaymate::TrackDef& track : song.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

}  // namespace

TEST_CASE("parseSongYaml accepts a valid manifest", "[song_yaml_parser]") {
    const auto songFolder = fixturesRoot() / "valid-song";
    const garageplaymate::Song song =
        garageplaymate::parseSongYaml(songYamlPath(songFolder), songFolder);

    CHECK(song.id == "valid-song");
    CHECK(song.title == "Valid Test Song");
    CHECK(song.bpm.has_value());
    CHECK(song.bpm.value() == 120);
    REQUIRE(song.timeSignature.has_value());
    CHECK(song.timeSignature->first == 4);
    CHECK(song.timeSignature->second == 4);
    CHECK(song.folderPath == songFolder);

    REQUIRE(song.tracks.size() == 2);

    const garageplaymate::TrackDef* drums = findTrack(song, "drums");
    REQUIRE(drums != nullptr);
    CHECK(drums->name == "Drums");
    CHECK(drums->folderPath == songFolder / "tracks/drums");
    CHECK(drums->defaultVolume == 1.0f);

    const garageplaymate::TrackDef* bass = findTrack(song, "bass");
    REQUIRE(bass != nullptr);
    CHECK(bass->name == "Bass");
    CHECK(bass->folderPath == songFolder / "tracks/bass");
    CHECK(bass->defaultVolume == 0.8f);

    REQUIRE(song.sections.size() == 3);
    CHECK(song.sections[0].id == "intro");
    CHECK(song.sections[0].startMs == 0);
    CHECK(song.sections[1].id == "verse");
    CHECK(song.sections[1].startMs == 15000);
    CHECK(song.sections[2].id == "chorus");
    CHECK(song.sections[2].startMs == 30000);
}

TEST_CASE("parseSongYaml rejects a manifest without id", "[song_yaml_parser]") {
    const auto songFolder = fixturesRoot() / "missing-id";

    REQUIRE_THROWS_AS(garageplaymate::parseSongYaml(songYamlPath(songFolder), songFolder),
                      garageplaymate::ParseError);
}

TEST_CASE("parseSongYaml rejects duplicate section ids", "[song_yaml_parser]") {
    const auto songFolder = fixturesRoot() / "duplicate-section";

    REQUIRE_THROWS_WITH(garageplaymate::parseSongYaml(songYamlPath(songFolder), songFolder),
                        Catch::Matchers::ContainsSubstring("Duplicate section id: intro"));
}
