#include "Logger.h"

#include <ctime>
#include <fstream>
#include <mutex>
#include <string>

namespace trueswing::rebuild::runtime {
namespace {

std::mutex g_logMutex;
std::filesystem::path g_logPath;

[[nodiscard]] std::string Timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &now);
    char value[32]{};
    std::strftime(value, sizeof(value), "%Y-%m-%d %H:%M:%S", &localTime);
    return value;
}

} // namespace

bool Logger::Initialize(const std::filesystem::path& directory) {
    return Initialize(directory, "TrueSwing.log",
                      "TrueSwing clean-rebuild session");
}

bool Logger::Initialize(const std::filesystem::path& directory,
                        const char* fileName, const char* sessionLabel) {
    std::scoped_lock lock(g_logMutex);
    if (fileName == nullptr || fileName[0] == '\0' ||
        sessionLabel == nullptr || sessionLabel[0] == '\0') {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        return false;
    }
    g_logPath = directory / fileName;
    std::ofstream stream(g_logPath, std::ios::app);
    if (stream) {
        stream << '[' << Timestamp() << "] === " << sessionLabel
               << " ===\n";
    }
    return static_cast<bool>(stream);
}

void Logger::Write(const char* message) {
    std::scoped_lock lock(g_logMutex);
    if (g_logPath.empty()) {
        return;
    }
    std::ofstream stream(g_logPath, std::ios::app);
    if (stream) {
        stream << '[' << Timestamp() << "] " << message << '\n';
    }
}

} // namespace trueswing::rebuild::runtime
