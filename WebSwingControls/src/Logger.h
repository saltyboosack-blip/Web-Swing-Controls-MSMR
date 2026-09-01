#pragma once

#include <filesystem>

namespace trueswing::rebuild::runtime {

class Logger final {
public:
    [[nodiscard]] static bool Initialize(
        const std::filesystem::path& directory);
    [[nodiscard]] static bool Initialize(
        const std::filesystem::path& directory, const char* fileName,
        const char* sessionLabel);
    static void Write(const char* message);
};

} // namespace trueswing::rebuild::runtime
