#include "GameBuild.h"

#include <Windows.h>
#include <bcrypt.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace trueswing::rebuild::runtime {
namespace {

constexpr DWORD kExpectedTimestamp = 0x6A43CDE7;
constexpr DWORD kExpectedImageSize = 0x085FD000;
constexpr char kExpectedTextSha256[] =
    "A956C11A814800F1427E32126C1BC78B1EEECAE3373C687F24397A4775AAF22F";
constexpr char kExpectedWholeFileSha256[] =
    "E297D4D94F1FFE4FEBF289745E79E7B6FA233A788E7A00F480FC77C55DB81AD1";

[[nodiscard]] std::string Hex(const std::array<std::byte, 32>& bytes) {
    char output[65]{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        sprintf_s(output + index * 2U, 3U, "%02X",
                  std::to_integer<unsigned char>(bytes[index]));
    }
    return output;
}

[[nodiscard]] bool HashSha256(const std::byte* data, std::size_t size,
                              std::array<std::byte, 32>& hash) {
    if (size > static_cast<std::size_t>(
                   std::numeric_limits<ULONG>::max())) {
        return false;
    }

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE handle = nullptr;
    DWORD objectSize = 0;
    DWORD returned = 0;
    std::vector<std::byte> object;
    bool ok = false;

    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                    nullptr, 0) != 0 ||
        BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&objectSize),
                          sizeof(objectSize), &returned, 0) != 0 ||
        returned != sizeof(objectSize)) {
        goto cleanup;
    }
    object.resize(objectSize);
    if (BCryptCreateHash(algorithm, &handle,
                         reinterpret_cast<PUCHAR>(object.data()), objectSize,
                         nullptr, 0, 0) != 0 ||
        BCryptHashData(handle,
                       reinterpret_cast<PUCHAR>(
                           const_cast<std::byte*>(data)),
                       static_cast<ULONG>(size), 0) != 0 ||
        BCryptFinishHash(handle, reinterpret_cast<PUCHAR>(hash.data()),
                         static_cast<ULONG>(hash.size()), 0) != 0) {
        goto cleanup;
    }
    ok = true;

cleanup:
    if (handle != nullptr) {
        BCryptDestroyHash(handle);
    }
    if (algorithm != nullptr) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }
    return ok;
}

} // namespace

bool GameBuild::IsSupported(std::string& reason) {
    wchar_t executablePath[32768]{};
    const DWORD length = GetModuleFileNameW(
        nullptr, executablePath, static_cast<DWORD>(std::size(executablePath)));
    if (length == 0 || length >= std::size(executablePath)) {
        reason = "could not resolve host executable path";
        return false;
    }
    return IsSupported(std::filesystem::path(executablePath), reason);
}

bool GameBuild::IsSupported(const std::filesystem::path& executablePath,
                            std::string& reason) {
    std::ifstream file(executablePath, std::ios::binary | std::ios::ate);
    if (!file) {
        reason = "could not open host executable";
        return false;
    }
    const std::streampos lengthBytes = file.tellg();
    if (lengthBytes <= 0) {
        reason = "host executable is empty";
        return false;
    }
    const auto unsignedLength = static_cast<std::uintmax_t>(lengthBytes);
    if (unsignedLength > static_cast<std::uintmax_t>(SIZE_MAX)) {
        reason = "host executable is too large";
        return false;
    }
    std::vector<std::byte> data(static_cast<std::size_t>(unsignedLength));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!file) {
        reason = "could not read host executable";
        return false;
    }
    std::array<std::byte, 32> wholeFileHash{};
    if (!HashSha256(data.data(), data.size(), wholeFileHash)) {
        reason = "could not hash complete host executable";
        return false;
    }
    if (Hex(wholeFileHash) != kExpectedWholeFileSha256) {
        reason = "unsupported complete host executable hash";
        return false;
    }
    if (data.size() < sizeof(IMAGE_DOS_HEADER)) {
        reason = "host executable is not a PE image";
        return false;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(data.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
        static_cast<std::size_t>(dos->e_lfanew) +
                sizeof(IMAGE_NT_HEADERS64) >
            data.size()) {
        reason = "host executable PE header is invalid";
        return false;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        data.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.SizeOfOptionalHeader <
            sizeof(IMAGE_OPTIONAL_HEADER64) ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        reason = "host executable is not a 64-bit PE image";
        return false;
    }
    if (nt->FileHeader.TimeDateStamp != kExpectedTimestamp) {
        reason = "unsupported executable timestamp";
        return false;
    }
    if (nt->OptionalHeader.SizeOfImage != kExpectedImageSize) {
        reason = "unsupported executable image size";
        return false;
    }

    const std::size_t sectionOffset =
        static_cast<std::size_t>(dos->e_lfanew) +
        offsetof(IMAGE_NT_HEADERS64, OptionalHeader) +
        nt->FileHeader.SizeOfOptionalHeader;
    const std::size_t sectionCount = nt->FileHeader.NumberOfSections;
    if (sectionCount == 0U || sectionOffset > data.size() ||
        sectionCount >
            (data.size() - sectionOffset) / sizeof(IMAGE_SECTION_HEADER)) {
        reason = "host executable section table is invalid";
        return false;
    }

    const IMAGE_SECTION_HEADER* text = nullptr;
    const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
    for (WORD index = 0; index < nt->FileHeader.NumberOfSections; ++index) {
        if (std::memcmp(sections[index].Name, ".text", 5U) == 0) {
            text = &sections[index];
            break;
        }
    }
    const std::size_t textOffset =
        text == nullptr ? 0U
                        : static_cast<std::size_t>(text->PointerToRawData);
    const std::size_t textSize =
        text == nullptr ? 0U : static_cast<std::size_t>(text->SizeOfRawData);
    if (text == nullptr || textOffset > data.size() ||
        textSize > data.size() - textOffset) {
        reason = "host executable .text section is invalid";
        return false;
    }

    std::array<std::byte, 32> hash{};
    if (!HashSha256(data.data() + textOffset, textSize, hash)) {
        reason = "could not hash host executable .text";
        return false;
    }
    if (Hex(hash) != kExpectedTextSha256) {
        reason = "unsupported executable .text hash";
        return false;
    }

    reason = "supported Steam build 23986256";
    return true;
}

} // namespace trueswing::rebuild::runtime
