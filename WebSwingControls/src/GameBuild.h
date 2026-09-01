#pragma once

#include <filesystem>
#include <string>

namespace trueswing::rebuild::runtime {

class GameBuild final {
public:
    [[nodiscard]] static bool IsSupported(std::string& reason);
    [[nodiscard]] static bool IsSupported(
        const std::filesystem::path& executablePath, std::string& reason);
};

} // namespace trueswing::rebuild::runtime
