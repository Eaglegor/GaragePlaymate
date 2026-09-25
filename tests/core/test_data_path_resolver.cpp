#include <catch2/catch_test_macros.hpp>

#include "storage/DataPathResolver.h"

#include <filesystem>

namespace {

using garageplaymate::DataPathResolver;
using garageplaymate::DistributionMode;

garageplaymate::PlatformPaths testPlatformPaths() {
    garageplaymate::PlatformPaths paths;
    paths.executableFile = std::filesystem::path("apps") / "GaragePlaymate" / "GaragePlaymate.exe";
    paths.documentsDir = std::filesystem::path("users") / "me" / "Documents";
    return paths;
}

}  // namespace

TEST_CASE("DataPathResolver default root depends on distribution mode", "[data_path_resolver]") {
    const auto paths = testPlatformPaths();
    CHECK(DataPathResolver::getDefaultDataRoot(DistributionMode::Portable, paths) ==
          std::filesystem::path("apps") / "GaragePlaymate" / "data");
    CHECK(DataPathResolver::getDefaultDataRoot(DistributionMode::Installed, paths) ==
          std::filesystem::path("users") / "me" / "Documents" / "GaragePlaymate");
}

TEST_CASE("DataPathResolver build default matches the compile-time mode", "[data_path_resolver]") {
    const auto paths = testPlatformPaths();
    CHECK(DataPathResolver::getDefaultDataRoot(paths) ==
          DataPathResolver::getDefaultDataRoot(DataPathResolver::getBuildDistributionMode(), paths));
}

TEST_CASE("DataPathResolver override wins over default", "[data_path_resolver]") {
    const auto paths = testPlatformPaths();
    const std::filesystem::path override = std::filesystem::path("OneDrive") / "Band";
    CHECK(DataPathResolver::getEffectiveDataRoot(override, paths) == override);
    CHECK(DataPathResolver::getEffectiveDataRoot(std::nullopt, paths) == DataPathResolver::getDefaultDataRoot(paths));
    CHECK(DataPathResolver::getEffectiveDataRoot(std::filesystem::path(), paths) ==
          DataPathResolver::getDefaultDataRoot(paths));
}

TEST_CASE("DataPathResolver derives songs and database paths", "[data_path_resolver]") {
    const std::filesystem::path root = std::filesystem::path("data-root");
    CHECK(DataPathResolver::getSongsDirectory(root) == root / "songs");
    CHECK(DataPathResolver::getDatabasePath(root) == root / "garageplaymate.db");
}
