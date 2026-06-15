#include <catch2/catch_test_macros.hpp>

#include "storage/SongYamlParser.h"

#include <filesystem>

namespace {

std::filesystem::path fixturesRoot() {
    return std::filesystem::path(TEST_FIXTURES_DIR) / "songs";
}

std::filesystem::path songYamlPath(const std::filesystem::path& songFolder) {
    return songFolder / "song.yaml";
}

}  // namespace

TEST_CASE("parseSongYaml accepts a valid manifest", "[song_yaml_parser]") {
    const auto songFolder = fixturesRoot() / "valid-song";
    const auto result = garageplaymate::parseSongYaml(songYamlPath(songFolder), songFolder);

    REQUIRE(result.has_value());
    const garageplaymate::Song& song = *result;

    CHECK(song.id == "valid-song");
    CHECK(song.title == "Valid Test Song");
    CHECK(song.bpm.has_value());
    CHECK(*song.bpm == 120);
    REQUIRE(song.timeSignature.has_value());
    CHECK(song.timeSignature->first == 4);
    CHECK(song.timeSignature->second == 4);
    CHECK(song.folderPath == songFolder);

    REQUIRE(song.tracks.size() == 2);
    CHECK(song.tracks[0].id == "drums");
    CHECK(song.tracks[0].name == "Drums");
    CHECK(song.tracks[0].folderPath == songFolder / "tracks/drums");
    CHECK(song.tracks[0].defaultVolume == 1.0f);
    CHECK(song.tracks[1].id == "bass");
    CHECK(song.tracks[1].name == "Bass");
    CHECK(song.tracks[1].folderPath == songFolder / "tracks/bass");
    CHECK(song.tracks[1].defaultVolume == 0.8f);

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
    const auto result = garageplaymate::parseSongYaml(songYamlPath(songFolder), songFolder);

    REQUIRE(!result.has_value());
    CHECK(result.error().message.find("id") != std::string::npos);
}

TEST_CASE("parseSongYaml rejects duplicate section ids", "[song_yaml_parser]") {
    const auto songFolder = fixturesRoot() / "duplicate-section";
    const auto result = garageplaymate::parseSongYaml(songYamlPath(songFolder), songFolder);

    REQUIRE(!result.has_value());
    CHECK(result.error().message == "Duplicate section id: intro");
}
